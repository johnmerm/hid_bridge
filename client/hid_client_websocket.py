#!/usr/bin/env python3
"""
HID Bridge WebSocket Client for Ubuntu

High-performance client using WebSocket with binary protocol.
Captures keyboard and mouse events and forwards them to ESP32 via WebSocket.

Binary Protocol:
- Keyboard: [0x01][action][key]
- Mouse Move: [0x02][x][y][wheel]
- Mouse Button: [0x03][action][button]

Actions: 0x01=press, 0x02=release, 0x03=click
"""

import argparse
import asyncio
import struct
import sys
import signal
from typing import Optional
import websockets
from evdev import InputDevice, categorize, ecodes, list_devices


# Protocol constants
MSG_KEYBOARD = 0x01
MSG_MOUSE_MOVE = 0x02
MSG_MOUSE_BUTTON = 0x03
MSG_STATUS = 0xFF

ACTION_PRESS = 0x01
ACTION_RELEASE = 0x02
ACTION_CLICK = 0x03


class HIDClientWebSocket:
    def __init__(self, esp32_ip: str, port: int = 81):
        self.esp32_ip = esp32_ip
        self.port = port
        self.ws_url = f"ws://{esp32_ip}:{port}"
        self.keyboard_device: Optional[InputDevice] = None
        self.mouse_device: Optional[InputDevice] = None
        self.running = True
        self.websocket = None

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

    async def send_keyboard_event(self, action: int, key: int):
        """Send keyboard event via WebSocket."""
        if self.websocket:
            try:
                packet = struct.pack('BBB', MSG_KEYBOARD, action, key)
                await self.websocket.send(packet)
            except Exception as e:
                print(f"Error sending keyboard event: {e}")

    async def send_mouse_move(self, x: int, y: int, wheel: int):
        """Send mouse move event via WebSocket."""
        if self.websocket:
            try:
                # Clamp to int8 range
                x = max(-127, min(127, x))
                y = max(-127, min(127, y))
                wheel = max(-127, min(127, wheel))

                packet = struct.pack('Bbbb', MSG_MOUSE_MOVE, x, y, wheel)
                await self.websocket.send(packet)
            except Exception as e:
                print(f"Error sending mouse move: {e}")

    async def send_mouse_button(self, action: int, button: int):
        """Send mouse button event via WebSocket."""
        if self.websocket:
            try:
                packet = struct.pack('BBB', MSG_MOUSE_BUTTON, action, button)
                await self.websocket.send(packet)
            except Exception as e:
                print(f"Error sending mouse button: {e}")

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
                    await self.send_keyboard_event(ACTION_PRESS, event.code)
                elif key_event.keystate == key_event.key_up:
                    await self.send_keyboard_event(ACTION_RELEASE, event.code)

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
                        await self.send_mouse_button(ACTION_PRESS, button)
                    elif event.value == 0:  # Release
                        await self.send_mouse_button(ACTION_RELEASE, button)

            # Send accumulated movement
            elif event.type == ecodes.EV_SYN:
                current_time = asyncio.get_event_loop().time()
                if (dx != 0 or dy != 0 or wheel != 0) and (current_time - last_send > 0.005):
                    await self.send_mouse_move(dx, dy, wheel)
                    dx, dy, wheel = 0, 0, 0
                    last_send = current_time

    async def connect_and_run(self):
        """Connect to WebSocket and run event handlers."""
        print(f"\nConnecting to WebSocket at {self.ws_url}...")

        try:
            async with websockets.connect(self.ws_url) as websocket:
                self.websocket = websocket
                print("✓ WebSocket connected!")

                # Wait for status message
                try:
                    status_msg = await asyncio.wait_for(websocket.recv(), timeout=2.0)
                    if len(status_msg) >= 3 and status_msg[0] == MSG_STATUS:
                        print(f"✓ Keyboard BLE: {'Connected' if status_msg[1] else 'Disconnected'}")
                        print(f"✓ Mouse BLE: {'Connected' if status_msg[2] else 'Disconnected'}")
                except asyncio.TimeoutError:
                    print("⚠ No status message received")

                print("\n" + "="*50)
                print("HID Bridge WebSocket Client Started")
                print("Press Ctrl+C to stop")
                print("="*50 + "\n")

                # Start event handlers
                tasks = []
                if self.keyboard_device:
                    tasks.append(asyncio.create_task(self.handle_keyboard_events()))
                if self.mouse_device:
                    tasks.append(asyncio.create_task(self.handle_mouse_events()))

                await asyncio.gather(*tasks)

        except Exception as e:
            print(f"✗ WebSocket connection error: {e}")
            print("\nMake sure:")
            print("  1. ESP32 is powered on")
            print("  2. Uploaded hid_bridge_websocket.ino")
            print("  3. ESP32 is connected to WiFi")
            print("  4. Using correct IP address")

    async def run(self):
        """Main entry point."""
        # Find input devices
        if not self.find_devices():
            print("\n✗ No input devices found!")
            return

        # Connect and run
        await self.connect_and_run()

    def stop(self):
        """Stop the client."""
        self.running = False
        print("\nStopping HID Bridge WebSocket Client...")


def main():
    parser = argparse.ArgumentParser(description='HID Bridge WebSocket Client for Ubuntu')
    parser.add_argument('--esp32-ip', required=True, help='ESP32 IP address')
    parser.add_argument('--port', type=int, default=81, help='WebSocket port (default: 81)')
    parser.add_argument('--keyboard', help='Path to keyboard device')
    parser.add_argument('--mouse', help='Path to mouse device')
    args = parser.parse_args()

    # Check if running as root
    import os
    if os.geteuid() != 0:
        print("✗ This script must be run as root to access input devices")
        print("  Try: sudo python3 hid_client_websocket.py --esp32-ip <IP>")
        sys.exit(1)

    client = HIDClientWebSocket(args.esp32_ip, args.port)

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
        print("\n✓ HID Bridge WebSocket Client stopped")


if __name__ == "__main__":
    main()
