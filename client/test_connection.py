#!/usr/bin/env python3
"""
Test script to verify ESP32 connection and send test events
"""

import argparse
import requests
import time
import sys


def test_connection(esp32_ip, port=80):
    """Test basic connectivity to ESP32."""
    base_url = f"http://{esp32_ip}:{port}"

    print(f"Testing connection to ESP32 at {base_url}...")

    # Test status endpoint
    try:
        response = requests.get(f"{base_url}/status", timeout=5)
        if response.status_code == 200:
            status = response.json()
            print("✓ ESP32 is reachable")
            print(f"  - IP: {status.get('ip')}")
            print(f"  - WiFi Connected: {status.get('wifi_connected')}")
            print(f"  - BLE Connected: {status.get('ble_connected')}")

            if not status.get('ble_connected'):
                print("\n⚠ Warning: BLE is not connected!")
                print("  Make sure your iPad is paired with 'ESP32 HID Bridge'")
                return False
            return True
        else:
            print(f"✗ Unexpected status code: {response.status_code}")
            return False
    except requests.exceptions.RequestException as e:
        print(f"✗ Connection failed: {e}")
        print("\nTroubleshooting:")
        print("  1. Check ESP32 is powered on")
        print("  2. Verify ESP32 is connected to WiFi (check serial monitor)")
        print("  3. Confirm you're using the correct IP address")
        print("  4. Ensure Ubuntu and ESP32 are on the same network")
        return False


def test_keyboard(esp32_ip, port=80):
    """Send test keyboard events."""
    base_url = f"http://{esp32_ip}:{port}"

    print("\n--- Testing Keyboard ---")

    test_cases = [
        ("Text input", {"action": "text", "text": "Hello from ESP32!"}),
        ("Key press 'A'", {"action": "press", "key": 0x04}),  # HID key code for 'A'
    ]

    for description, payload in test_cases:
        print(f"  {description}...", end=" ")
        try:
            response = requests.post(f"{base_url}/keyboard", json=payload, timeout=2)
            if response.status_code == 200:
                print("✓")
                time.sleep(0.5)
            else:
                print(f"✗ (status: {response.status_code})")
                print(f"    Response: {response.text}")
        except requests.exceptions.RequestException as e:
            print(f"✗ ({e})")


def test_mouse(esp32_ip, port=80):
    """Send test mouse events."""
    base_url = f"http://{esp32_ip}:{port}"

    print("\n--- Testing Mouse ---")

    test_cases = [
        ("Move right", {"action": "move", "x": 50, "y": 0}),
        ("Move down", {"action": "move", "x": 0, "y": 50}),
        ("Move left", {"action": "move", "x": -50, "y": 0}),
        ("Move up", {"action": "move", "x": 0, "y": -50}),
        ("Left click", {"action": "click", "button": 1}),
    ]

    for description, payload in test_cases:
        print(f"  {description}...", end=" ")
        try:
            response = requests.post(f"{base_url}/mouse", json=payload, timeout=2)
            if response.status_code == 200:
                print("✓")
                time.sleep(0.5)
            elif response.status_code == 501:
                print("⚠ (Mouse not implemented - use combo version)")
                break
            else:
                print(f"✗ (status: {response.status_code})")
                print(f"    Response: {response.text}")
        except requests.exceptions.RequestException as e:
            print(f"✗ ({e})")


def main():
    parser = argparse.ArgumentParser(description='Test ESP32 HID Bridge connection')
    parser.add_argument('--esp32-ip', required=True, help='ESP32 IP address')
    parser.add_argument('--port', type=int, default=80, help='ESP32 server port (default: 80)')
    parser.add_argument('--skip-keyboard', action='store_true', help='Skip keyboard tests')
    parser.add_argument('--skip-mouse', action='store_true', help='Skip mouse tests')
    args = parser.parse_args()

    print("="*60)
    print("ESP32 HID Bridge - Connection Test")
    print("="*60)

    # Test basic connection
    if not test_connection(args.esp32_ip, args.port):
        print("\n✗ Connection test failed!")
        sys.exit(1)

    print("\n✓ Connection successful!")

    # Test keyboard
    if not args.skip_keyboard:
        proceed = input("\nTest keyboard? This will send test keystrokes to your iPad [y/N]: ")
        if proceed.lower() == 'y':
            test_keyboard(args.esp32_ip, args.port)

    # Test mouse
    if not args.skip_mouse:
        proceed = input("\nTest mouse? This will move the cursor on your iPad [y/N]: ")
        if proceed.lower() == 'y':
            test_mouse(args.esp32_ip, args.port)

    print("\n" + "="*60)
    print("Test complete!")
    print("="*60)


if __name__ == "__main__":
    main()
