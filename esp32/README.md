# ESP32 HID Bridge Firmware

## Available Firmware Options

### Option 1: hid_bridge.ino (Recommended - Keyboard + Mouse)
Uses separate BLE Keyboard and BLE Mouse libraries for full functionality.

### Option 2: hid_bridge_combo.ino (Alternative - Combo Library)
Uses the ESP32-BLE-Combo library for combined keyboard and mouse support.

## Required Libraries

### For hid_bridge.ino (Recommended):

1. **ESP32 Board Support**
   - Go to File → Preferences
   - Add to Additional Board URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to Tools → Board → Board Manager
   - Search for "esp32" and install

2. **ESP32 BLE Keyboard**
   - Library Manager: Search "ESP32 BLE Keyboard" by T-vK
   - Install latest version

3. **ESP32 BLE Mouse**
   - Library Manager: Search "ESP32 BLE Mouse" by T-vK
   - Install latest version

4. **ArduinoJson**
   - Library Manager: Search "ArduinoJson" by Benoit Blanchon
   - Install version 6.x

### For hid_bridge_combo.ino (Alternative):

1. **ESP32 Board Support** (same as above)

2. **ESP32-BLE-Combo** (for keyboard + mouse in one library)
   - Download: https://github.com/blackketter/ESP32-BLE-Combo
   - Extract to Arduino/libraries folder
   - OR use Library Manager: Search "ESP32-BLE-Combo"

3. **ArduinoJson** (same as above)

## Configuration

1. Edit `config.h` or modify directly in the .ino file:
   ```cpp
   const char* ssid = "YourWiFiName";
   const char* password = "YourWiFiPassword";
   ```

2. Select your board:
   - Tools → Board → ESP32 Dev Module (or your specific board)

3. Upload the sketch

## Pinout

- **LED Pin**: GPIO2 (built-in LED on most boards)
  - Slow blink (2s): BLE connected
  - Fast blink: WiFi connecting
  - Solid: WiFi connected, waiting for BLE

## Troubleshooting

### WiFi Won't Connect
- Check SSID and password
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check serial monitor for IP address

### BLE Won't Pair
- Only one device can connect at a time
- Forget the device on iPad and re-pair
- Reset ESP32 and try again
- Check serial monitor for errors

### Out of Memory Errors
- Reduce JSON buffer sizes in code
- Use ESP32 with PSRAM if available

## Serial Monitor

After uploading, open Serial Monitor (115200 baud) to see:
- WiFi connection status
- IP address
- BLE connection status

## LED Status Indicators

| Pattern | Meaning |
|---------|---------|
| Fast blink | Connecting to WiFi |
| Solid ON | WiFi connected, waiting for BLE |
| Slow blink (2s) | BLE connected and ready |
