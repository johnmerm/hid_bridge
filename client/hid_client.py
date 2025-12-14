#!/usr/bin/env python3
"""
HID Bridge Client for Ubuntu

Captures keyboard and mouse events and forwards them to ESP32 via HTTP.
Requires root privileges to access input devices.
"""

import argparse
import asyncio
import json
import sys
import signal
from typing import Optional
import requests
from evdev import InputDevice, categorize, ecodes, list_devices


class HIDClient:
    def __init__(self, esp32_ip: str, port: int = 80):
        self.esp32_ip = esp32_ip
        self.port = port
        self.base_url = f"http://{esp32_ip}:{port}"
        self.keyboard_device: Optional[InputDevice] = None
        self.mouse_device: Optional[InputDevice] = None
        self.running = True

    def check_esp32_status(self) -> bool:
        """Check if ESP32 is reachable and BLE is connected."""
        try:
            response = requests.get(f"{self.base_url}/status", timeout=2)
            if response.status_code == 200:
                status = response.json()
                print(f"✓ ESP32 connected: {status['ip']}")
                print(f"✓ BLE connected: {status['ble_connected']}")
                return status['ble_connected']
            return False
        except requests.exceptions.RequestException as e:
            print(f"✗ Cannot connect to ESP32: {e}")
            return False

    def find_devices(self):
        """Find keyboard and mouse input devices."""
        devices = [InputDevice(path) for path in list_devices()]

        print("\nAvailable input devices:")
        for idx, device in enumerate(devices):
            print(f"{idx}: {device.name} ({device.path})")
            caps = device.capabilities(verbose=True)
            # Print capabilities for debugging
            for cap_type, cap_codes in caps.items():
                if cap_type[0] == 'EV_KEY':
                    print(f"   - {cap_type[1]}")

        # Try to auto-detect keyboard and mouse
        for device in devices:
            caps = device.capabilities()

            # Check for keyboard (has letter keys)
            if ecodes.EV_KEY in caps:
                keys = caps[ecodes.EV_KEY]
                # Check if it has typical keyboard keys
                if ecodes.KEY_A in keys or ecodes.KEY_ENTER in keys:
                    if not self.keyboard_device:
                        self.keyboard_device = device
                        print(f"\n✓ Using keyboard: {device.name}")

            # Check for mouse (has relative movement)
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

    def send_keyboard_event(self, action: str, key: int):
        """Send keyboard event to ESP32."""
        try:
            payload = {
                "action": action,
                "key": key
            }
            requests.post(
                f"{self.base_url}/keyboard",
                json=payload,
                timeout=0.5
            )
        except requests.exceptions.RequestException as e:
            print(f"Error sending keyboard event: {e}")

    def send_mouse_event(self, action: str, **kwargs):
        """Send mouse event to ESP32."""
        try:
            payload = {"action": action, **kwargs}
            requests.post(
                f"{self.base_url}/mouse",
                json=payload,
                timeout=0.5
            )
        except requests.exceptions.RequestException as e:
            print(f"Error sending mouse event: {e}")

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
                    print(f"Key down: {key_event.keycode} ({event.code})")
                    self.send_keyboard_event("down", event.code)
                elif key_event.keystate == key_event.key_up:
                    print(f"Key up: {key_event.keycode} ({event.code})")
                    self.send_keyboard_event("up", event.code)

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
                    ecodes.BTN_LEFT: 1,    # MOUSE_LEFT
                    ecodes.BTN_RIGHT: 2,   # MOUSE_RIGHT
                    ecodes.BTN_MIDDLE: 4,  # MOUSE_MIDDLE
                }

                if event.code in button_map:
                    button = button_map[event.code]
                    if event.value == 1:  # Press
                        print(f"Mouse button {button} pressed")
                        self.send_mouse_event("press", button=button)
                    elif event.value == 0:  # Release
                        print(f"Mouse button {button} released")
                        self.send_mouse_event("release", button=button)

            # Send accumulated movement periodically
            elif event.type == ecodes.EV_SYN:
                current_time = asyncio.get_event_loop().time()
                if (dx != 0 or dy != 0 or wheel != 0) and (current_time - last_send > 0.01):
                    # Clamp values to int8 range (-127 to 127)
                    dx_clamped = max(-127, min(127, dx))
                    dy_clamped = max(-127, min(127, dy))
                    wheel_clamped = max(-127, min(127, wheel))

                    self.send_mouse_event(
                        "move",
                        x=dx_clamped,
                        y=dy_clamped,
                        wheel=wheel_clamped
                    )
                    dx, dy, wheel = 0, 0, 0
                    last_send = current_time

    async def run(self):
        """Main event loop."""
        # Check ESP32 connection
        if not self.check_esp32_status():
            print("\n⚠ Warning: ESP32 not ready. Make sure:")
            print("  1. ESP32 is powered on")
            print("  2. Connected to the same WiFi network")
            print("  3. iPad is paired via Bluetooth")
            response = input("\nContinue anyway? [y/N]: ")
            if response.lower() != 'y':
                return

        # Find input devices
        if not self.find_devices():
            print("\n✗ No input devices found!")
            return

        print("\n" + "="*50)
        print("HID Bridge Client Started")
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

    def stop(self):
        """Stop the client."""
        self.running = False
        print("\nStopping HID Bridge Client...")


def main():
    parser = argparse.ArgumentParser(description='HID Bridge Client for Ubuntu')
    parser.add_argument('--esp32-ip', required=True, help='ESP32 IP address')
    parser.add_argument('--port', type=int, default=80, help='ESP32 server port (default: 80)')
    parser.add_argument('--keyboard', help='Path to keyboard device (e.g., /dev/input/event3)')
    parser.add_argument('--mouse', help='Path to mouse device (e.g., /dev/input/event4)')
    args = parser.parse_args()

    # Check if running as root
    import os
    if os.geteuid() != 0:
        print("✗ This script must be run as root to access input devices")
        print("  Try: sudo python3 hid_client.py --esp32-ip <IP>")
        sys.exit(1)

    client = HIDClient(args.esp32_ip, args.port)

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
        print("\n✓ HID Bridge Client stopped")


if __name__ == "__main__":
    main()
