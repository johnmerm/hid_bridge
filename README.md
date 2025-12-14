# HID Bridge

Transfer keyboard and mouse events from Ubuntu to iPad via ESP32 Bluetooth HID.

## Architecture

```
Ubuntu Laptop → WiFi/HTTP → ESP32 → Bluetooth HID → iPad
```

### Components

1. **Ubuntu Client** (`client/`) - Captures keyboard and mouse events
2. **ESP32 Bridge** (`esp32/`) - WiFi server + Bluetooth HID device
3. **Communication Protocol** - HTTP REST API for event transmission

## Features

- ✅ Keyboard event forwarding
- ✅ Mouse event forwarding (movement, clicks, scroll)
- ✅ Low latency transmission over WiFi
- ✅ Bluetooth HID compatibility with iPad

## Hardware Requirements

- ESP32 development board (ESP32-WROOM, ESP32-DevKit, etc.)
- Ubuntu laptop with WiFi
- iPad with Bluetooth

## Software Requirements

### ESP32
- Arduino IDE or PlatformIO
- ESP32 BLE HID library

### Ubuntu
- Python 3.7+
- Required packages: `evdev`, `requests`

## Quick Start

### 1. Setup ESP32

1. Open `esp32/hid_bridge/hid_bridge.ino` in Arduino IDE
2. Install required libraries:
   - ESP32 BLE HID by Neil Kolban
3. Configure WiFi credentials in the code
4. Upload to ESP32
5. Note the IP address shown in Serial Monitor

### 2. Setup Ubuntu Client

```bash
cd client
pip install -r requirements.txt
sudo python3 hid_client.py --esp32-ip <ESP32_IP_ADDRESS>
```

### 3. Connect iPad

1. Go to Settings → Bluetooth on iPad
2. Look for "ESP32 HID Bridge"
3. Connect to it
4. Start sending events from Ubuntu!

## Configuration

### ESP32 Configuration
Edit `esp32/hid_bridge/config.h`:
- WiFi SSID and password
- Device name for Bluetooth
- Server port

### Client Configuration
Edit `client/config.json`:
- ESP32 IP address
- Input device paths
- Key mappings

## Protocol

The client sends HTTP POST requests to ESP32:

```json
{
  "type": "keyboard",
  "action": "press",
  "key": 0x04
}
```

```json
{
  "type": "mouse",
  "action": "move",
  "x": 10,
  "y": -5
}
```

## Troubleshooting

- **ESP32 not showing in Bluetooth**: Check serial monitor for errors, ensure BLE is initialized
- **High latency**: Use 5GHz WiFi if possible, reduce distance to ESP32
- **Events not captured**: Run client with `sudo` for device access

## License

MIT

## Contributing

Pull requests welcome! Please test on your hardware before submitting.
