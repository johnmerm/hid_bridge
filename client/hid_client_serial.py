#!/usr/bin/env python3
"""
HID Bridge Serial (USB) Client for Ubuntu

Lowest latency client using direct USB serial connection.
No WiFi required - ESP32 connects to Ubuntu via USB cable.

Binary Protocol:
- Keyboard: [0x01][action][key]
- Mouse Move: [0x02][x][y][wheel]
- Mouse Button: [0x03][action][button]

Actions: 0x01=press, 0x02=release, 0x03=click

Advantages:
- Ultra-low latency (~1-2ms)
- No WiFi setup required
- More reliable than wireless
- No network interference
"""

import argparse
import asyncio
import struct
import sys
import signal
from typing import Optional
import serial
import serial.tools.list_ports
from evdev import InputDevice, categorize, ecodes, list_devices


# Protocol constants
MSG_KEYBOARD = 0x01
MSG_MOUSE_MOVE = 0x02
MSG_MOUSE_BUTTON = 0x03
MSG_STATUS_REQUEST = 0xFE
MSG_STATUS_RESPONSE = 0xFF

ACTION_PRESS = 0x01
ACTION_RELEASE = 0x02
ACTION_CLICK = 0x03


class HIDClientSerial:
    def __init__(self, serial_port: str, baud_rate: int = 921600):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.keyboard_device: Optional[InputDevice] = None
        self.mouse_device: Optional[InputDevice] = None
        self.running = True
        self.ser = None

    @staticmethod
    def list_serial_ports():
        """List available serial ports."""
        ports = serial.tools.list_ports.comports()
        print("\nAvailable serial ports:")
        for port in ports:
            print(f"  {port.device} - {port.description}")
            if "USB" in port.description or "ESP32" in port.description:
                print(f"    ^ This looks like an ESP32!")
        return [port.device for port in ports]

    def find_devices(self):
        """Find keyboard and mouse input devices."""
        devices = [InputDevice(path) for path in list_devices()]

        print("\nAvailable input devices:")
        for idx, device in enumerate(devices):
            print(f"{idx}: {device.name} ({device.path})")

        # Try to auto-detect keyboard and mouse
        for device in devices:
            caps = device.capabilities()

            # Check for keyboard
            if ecodes.EV_KEY in caps:
                keys = caps[ecodes.EV_KEY]
                if ecodes.KEY_A in keys or ecodes.KEY_ENTER in keys:
                    if not self.keyboard_device:
                        self.keyboard_device = device
                        print(f"\n✓ Using keyboard: {device.name}")

            # Check for mouse
            if ecodes.EV_REL in caps:
                if ecodes.REL_X in caps[ecodes.EV_REL]:
                    if not self.mouse_device:
                        self.mouse_device = device
                        print(f"✓ Using mouse: {device.name}")

        if not self.keyboard_device:
            print("\n⚠ No keyboard detected")
        if not self.mouse_device:
            print("⚠ No mouse detected")

        return self.keyboard_device or self.mouse_device

    def check_esp32_status(self) -> bool:
        """Check if ESP32 is connected and responding."""
        try:
            # Send status request
            self.ser.write(bytes([MSG_STATUS_REQUEST]))
            self.ser.flush()

            # Wait for response (with timeout)
            import time
            start_time = time.time()
            buffer = bytearray()

            while time.time() - start_time < 2.0:
                if self.ser.in_waiting > 0:
                    buffer.extend(self.ser.read(self.ser.in_waiting))

                    # Check for status response
                    if len(buffer) >= 3 and buffer[0] == MSG_STATUS_RESPONSE:
                        print(f"✓ ESP32 connected via {self.serial_port}")
                        print(f"✓ Keyboard BLE: {'Connected' if buffer[1] else 'Disconnected'}")
                        print(f"✓ Mouse BLE: {'Connected' if buffer[2] else 'Disconnected'}")
                        return True

                time.sleep(0.01)

            print("⚠ No response from ESP32")
            return False

        except Exception as e:
            print(f"✗ Error checking status: {e}")
            return False

    def send_packet(self, packet: bytes):
        """Send packet via serial."""
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(packet)
                # No flush needed for high-speed serial - adds latency
            except Exception as e:
                print(f"Error sending packet: {e}")

    def send_keyboard_event(self, action: int, key: int):
        """Send keyboard event via serial."""
        packet = struct.pack('BBB', MSG_KEYBOARD, action, key)
        self.send_packet(packet)

    def send_mouse_move(self, x: int, y: int, wheel: int):
        """Send mouse move event via serial."""
        # Clamp to int8 range
        x = max(-127, min(127, x))
        y = max(-127, min(127, y))
        wheel = max(-127, min(127, wheel))

        packet = struct.pack('Bbbb', MSG_MOUSE_MOVE, x, y, wheel)
        self.send_packet(packet)

    def send_mouse_button(self, action: int, button: int):
        """Send mouse button event via serial."""
        packet = struct.pack('BBB', MSG_MOUSE_BUTTON, action, button)
        self.send_packet(packet)

    async def handle_keyboard_events(self):
        """Handle keyboard input events."""
        if not self.keyboard_device:
            return

        print(f"Listening for keyboard events on {self.keyboard_device.name}")
        async for event in self.keyboard_device.async_read_loop():
            if not self.running:
                break

            if event.type == ecodes.EV_KEY:
                key_event = categorize(event)
                if key_event.keystate == key_event.key_down:
                    self.send_keyboard_event(ACTION_PRESS, event.code)
                elif key_event.keystate == key_event.key_up:
                    self.send_keyboard_event(ACTION_RELEASE, event.code)

    async def handle_mouse_events(self):
        """Handle mouse input events."""
        if not self.mouse_device:
            return

        print(f"Listening for mouse events on {self.mouse_device.name}")

        # Accumulate relative movements
        dx, dy, wheel = 0, 0, 0
        last_send = asyncio.get_event_loop().time()

        async for event in self.mouse_device.async_read_loop():
            if not self.running:
                break

            # Relative movement
            if event.type == ecodes.EV_REL:
                if event.code == ecodes.REL_X:
                    dx += event.value
                elif event.code == ecodes.REL_Y:
                    dy += event.value
                elif event.code == ecodes.REL_WHEEL:
                    wheel += event.value

            # Mouse buttons
            elif event.type == ecodes.EV_KEY:
                button_map = {
                    ecodes.BTN_LEFT: 1,
                    ecodes.BTN_RIGHT: 2,
                    ecodes.BTN_MIDDLE: 4,
                }

                if event.code in button_map:
                    button = button_map[event.code]
                    if event.value == 1:  # Press
                        self.send_mouse_button(ACTION_PRESS, button)
                    elif event.value == 0:  # Release
                        self.send_mouse_button(ACTION_RELEASE, button)

            # Send accumulated movement (very frequently for serial - lowest latency)
            elif event.type == ecodes.EV_SYN:
                current_time = asyncio.get_event_loop().time()
                if (dx != 0 or dy != 0 or wheel != 0) and (current_time - last_send > 0.001):
                    self.send_mouse_move(dx, dy, wheel)
                    dx, dy, wheel = 0, 0, 0
                    last_send = current_time

    async def run(self):
        """Main event loop."""
        # Open serial port
        print(f"\nOpening serial port: {self.serial_port} at {self.baud_rate} baud...")
        try:
            self.ser = serial.Serial(
                port=self.serial_port,
                baudrate=self.baud_rate,
                timeout=1,
                write_timeout=0  # Non-blocking writes for lowest latency
            )
            print("✓ Serial port opened")

            # Wait for ESP32 to be ready
            print("\nWaiting for ESP32 to initialize...")
            import time
            time.sleep(2)  # Give ESP32 time to boot

            # Check status
            if not self.check_esp32_status():
                print("\n⚠ Warning: ESP32 not responding properly")
                print("  Make sure you uploaded hid_bridge_serial.ino")
                response = input("\nContinue anyway? [y/N]: ")
                if response.lower() != 'y':
                    return

        except serial.SerialException as e:
            print(f"✗ Failed to open serial port: {e}")
            print("\nTroubleshooting:")
            print("  1. Make sure ESP32 is connected via USB")
            print("  2. Check if the port is correct")
            print("  3. Try: sudo usermod -a -G dialout $USER (then logout/login)")
            print("  4. Run with sudo if necessary")
            return

        # Find input devices
        if not self.find_devices():
            print("\n✗ No input devices found!")
            return

        print("\n" + "="*50)
        print("HID Bridge Serial Client Started")
        print(f"Port: {self.serial_port} @ {self.baud_rate} baud")
        print("Press Ctrl+C to stop")
        print("="*50 + "\n")

        # Start event handlers
        tasks = []
        if self.keyboard_device:
            tasks.append(asyncio.create_task(self.handle_keyboard_events()))
        if self.mouse_device:
            tasks.append(asyncio.create_task(self.handle_mouse_events()))

        try:
            await asyncio.gather(*tasks)
        except asyncio.CancelledError:
            pass
        finally:
            if self.ser and self.ser.is_open:
                self.ser.close()
                print("✓ Serial port closed")

    def stop(self):
        """Stop the client."""
        self.running = False
        print("\nStopping HID Bridge Serial Client...")


def main():
    parser = argparse.ArgumentParser(description='HID Bridge Serial Client for Ubuntu')
    parser.add_argument('--port', help='Serial port (e.g., /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=921600, help='Baud rate (default: 921600)')
    parser.add_argument('--list-ports', action='store_true', help='List available serial ports')
    parser.add_argument('--keyboard', help='Path to keyboard device')
    parser.add_argument('--mouse', help='Path to mouse device')
    args = parser.parse_args()

    # List ports if requested
    if args.list_ports:
        HIDClientSerial.list_serial_ports()
        sys.exit(0)

    # Check if port is specified
    if not args.port:
        print("✗ Serial port not specified")
        print("\nAvailable ports:")
        ports = HIDClientSerial.list_serial_ports()
        if ports:
            print(f"\nTry: sudo python3 hid_client_serial.py --port {ports[0]}")
        else:
            print("\n✗ No serial ports found! Is the ESP32 connected?")
        sys.exit(1)

    # Check if running as root
    import os
    if os.geteuid() != 0:
        print("✗ This script must be run as root to access input devices")
        print(f"  Try: sudo python3 hid_client_serial.py --port {args.port}")
        sys.exit(1)

    client = HIDClientSerial(args.port, args.baud)

    # Override device selection if specified
    if args.keyboard:
        client.keyboard_device = InputDevice(args.keyboard)
    if args.mouse:
        client.mouse_device = InputDevice(args.mouse)

    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        client.stop()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # Run the client
    try:
        asyncio.run(client.run())
    except KeyboardInterrupt:
        pass
    finally:
        print("\n✓ HID Bridge Serial Client stopped")


if __name__ == "__main__":
    main()
