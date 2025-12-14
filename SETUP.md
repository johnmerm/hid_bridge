# Setup Guide

Complete setup instructions for the HID Bridge project.

## Overview

This project allows you to control your iPad using your Ubuntu laptop's keyboard and mouse via an ESP32 bridge device.

```
┌─────────────┐         ┌─────────┐         ┌──────────┐
│   Ubuntu    │  WiFi   │  ESP32  │   BLE   │   iPad   │
│   Laptop    ├────────►│ Bridge  ├────────►│          │
│             │  HTTP   │         │   HID   │          │
└─────────────┘         └─────────┘         └──────────┘
```

## Hardware Requirements

- **ESP32 Development Board**
  - ESP32-WROOM-32
  - ESP32 DevKit V1
  - Or any ESP32 board with BLE support
  - USB cable for programming

- **Ubuntu Laptop**
  - WiFi capability
  - Python 3.7+

- **iPad**
  - Any iPad with Bluetooth

## Software Setup

### 1. ESP32 Setup (15 minutes)

#### Install Arduino IDE

```bash
# Download from https://www.arduino.cc/en/software
# Or use snap:
sudo snap install arduino
```

#### Configure Arduino IDE

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search "esp32" and install "esp32 by Espressif Systems"

#### Install Required Libraries

1. Go to **Sketch → Include Library → Manage Libraries**
2. Install these libraries:
   - **ESP32-BLE-Combo** (for keyboard + mouse)
     - Search: "ESP32-BLE-Combo" by blackketter
   - **ArduinoJson** version 6.x
     - Search: "ArduinoJson" by Benoit Blanchon

   *Alternative: If you only need keyboard support, install "ESP32 BLE Keyboard" instead*

#### Upload Firmware

1. Open `esp32/hid_bridge/hid_bridge_combo.ino` (or `hid_bridge.ino` for keyboard only)

2. **Configure WiFi**: Edit these lines at the top of the file:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

3. **Select Board**: Tools → Board → ESP32 Dev Module (or your specific board)

4. **Select Port**: Tools → Port → /dev/ttyUSB0 (or similar)

5. **Upload**: Click the Upload button (→)

6. **Open Serial Monitor**: Tools → Serial Monitor (set to 115200 baud)

7. **Note the IP Address**: After upload, the serial monitor will show:
   ```
   WiFi connected!
   IP address: 192.168.1.XXX
   ```
   **Write down this IP address - you'll need it for the Ubuntu client!**

### 2. Ubuntu Client Setup (5 minutes)

#### Install Python Dependencies

```bash
cd client

# Option 1: Using pip
pip install -r requirements.txt

# Option 2: System-wide
sudo apt-get update
sudo apt-get install python3-pip
sudo pip3 install evdev requests
```

#### Test Connection

```bash
# Replace with your ESP32's IP address
sudo python3 test_connection.py --esp32-ip 192.168.1.XXX
```

Expected output:
```
✓ ESP32 is reachable
  - IP: 192.168.1.XXX
  - WiFi Connected: True
  - BLE Connected: False  ← Will be True after iPad pairs
```

### 3. iPad Setup (2 minutes)

1. Open **Settings → Bluetooth** on iPad
2. Wait for "ESP32 HID Bridge" to appear in "Other Devices"
3. Tap to connect
4. No PIN required - it should connect immediately

You should now see in the Ubuntu test script:
```
  - BLE Connected: True
```

## First Test

### Test from Ubuntu

```bash
# Run the HID client
sudo python3 hid_client.py --esp32-ip 192.168.1.XXX
```

You should see:
```
✓ ESP32 connected: 192.168.1.XXX
✓ BLE connected: True

Available input devices:
0: AT Translated Set 2 keyboard (/dev/input/event3)
1: Logitech USB Mouse (/dev/input/event4)

✓ Using keyboard: AT Translated Set 2 keyboard
✓ Using mouse: Logitech USB Mouse

HID Bridge Client Started
Press Ctrl+C to stop
```

### Test Input

1. **Type on your Ubuntu laptop** - text should appear on iPad
2. **Move your mouse** - cursor should move on iPad
3. **Click** - clicks should register on iPad

## Troubleshooting

### ESP32 Won't Connect to WiFi

**Problem**: Serial monitor shows "WiFi connection failed"

**Solutions**:
- Verify SSID and password are correct
- Ensure you're using 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check WiFi signal strength near ESP32
- Try another WiFi network

### iPad Won't Pair

**Problem**: "ESP32 HID Bridge" doesn't appear in Bluetooth settings

**Solutions**:
- Check ESP32 serial monitor - should show "Starting BLE HID"
- Reset ESP32 (press reset button)
- If iPad was previously paired, "Forget This Device" and re-pair
- Make sure no other device is connected to the ESP32 (only one at a time)

### Client Shows "Permission Denied"

**Problem**: Can't access input devices

**Solution**:
```bash
# Must run as root
sudo python3 hid_client.py --esp32-ip 192.168.1.XXX
```

### High Latency

**Problem**: Noticeable delay between input and response

**Solutions**:
- Use 5GHz WiFi for Ubuntu/ESP32 (ESP32 on 2.4GHz, but reduce interference)
- Move ESP32 closer to WiFi router
- Reduce distance between ESP32 and iPad
- Check WiFi network isn't congested

### Wrong Keyboard Layout

**Problem**: Keys produce wrong characters

**Solution**:
- Change iPad keyboard layout: Settings → General → Keyboard → Keyboards
- Add "English (US)" or your preferred layout
- Or modify key mappings in `hid_client.py`

### Mouse Not Working

**Problem**: Keyboard works but mouse doesn't

**Solution**:
- Make sure you're using `hid_bridge_combo.ino` (not `hid_bridge.ino`)
- Verify ESP32-BLE-Combo library is installed
- Check serial monitor for errors

## Advanced Configuration

### Auto-start on Boot

Create systemd service on Ubuntu:

```bash
sudo nano /etc/systemd/system/hid-bridge.service
```

Add:
```ini
[Unit]
Description=HID Bridge Client
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /home/YOUR_USER/hid_bridge/client/hid_client.py --esp32-ip 192.168.1.XXX
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

Enable:
```bash
sudo systemctl enable hid-bridge
sudo systemctl start hid-bridge
```

### Static IP for ESP32

In the Arduino code, before `WiFi.begin()`:

```cpp
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

if (!WiFi.config(local_IP, gateway, subnet)) {
  Serial.println("STA Failed to configure");
}
```

## Next Steps

- Test all functionality (keyboard, mouse, clicks)
- Adjust mouse sensitivity if needed
- Set up auto-start if desired
- Consider adding battery to ESP32 for portability

## Getting Help

If you encounter issues:
1. Check the serial monitor for ESP32 errors
2. Review the troubleshooting section above
3. Open an issue on GitHub with:
   - Your ESP32 board model
   - Ubuntu version
   - Serial monitor output
   - Error messages
