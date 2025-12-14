# ESP32 HID Bridge Firmware

## Required Libraries

Install these libraries in Arduino IDE:

1. **ESP32 Board Support**
   - Go to File → Preferences
   - Add to Additional Board URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to Tools → Board → Board Manager
   - Search for "esp32" and install

2. **BLE Combo Library** (for keyboard + mouse)
   - Download: https://github.com/blackketter/ESP32-BLE-Combo
   - Extract to Arduino/libraries folder
   - OR use Library Manager: Search "ESP32-BLE-Combo"

3. **ArduinoJson**
   - Library Manager: Search "ArduinoJson" by Benoit Blanchon
   - Install version 6.x

## Alternative: BLE Keyboard Only

If you only need keyboard support (simpler):

1. **ESP32 BLE Keyboard**
   - Library Manager: Search "ESP32 BLE Keyboard"
   - Use `hid_bridge.ino` instead of `hid_bridge_combo.ino`

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
