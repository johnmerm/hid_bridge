# HID Bridge

Transfer keyboard and mouse events from Ubuntu to iPad via ESP32 Bluetooth HID.

## Architecture

```
Ubuntu Laptop → Protocol → ESP32 → Bluetooth HID → iPad
```

### Components

1. **Ubuntu Client** (`client/`) - Captures keyboard and mouse events
2. **ESP32 Bridge** (`esp32/`) - Protocol server + Bluetooth HID device
3. **Communication Protocols** - Multiple options for different performance needs

## Protocol Options

HID Bridge supports **4 different communication protocols**, each optimized for different use cases:

| Protocol | Latency | Best For |
|----------|---------|----------|
| **HTTP/JSON** | 10-50ms | Development, testing |
| **WebSocket Binary** | 2-10ms | **Recommended** - Best balance |
| **UDP Binary** | 1-5ms | Competitive gaming, lowest latency |
| **Serial/USB** | 1-2ms | **Best performance** - wired connection |

See [PROTOCOLS.md](PROTOCOLS.md) for detailed comparison.

## Features

- ✅ Keyboard event forwarding (all keys including modifiers)
- ✅ Mouse event forwarding (movement, clicks, scroll)
- ✅ Multiple protocol options (HTTP, WebSocket, UDP, Serial)
- ✅ Ultra-low latency (1-2ms with Serial/USB)
- ✅ Binary protocol for minimal overhead
- ✅ Bluetooth HID compatibility with iPad/iPhone/Mac
- ✅ Auto-detection of input devices

## Hardware Requirements

- ESP32 development board (ESP32-WROOM, ESP32-DevKit, etc.)
- Ubuntu laptop with WiFi
- iPad with Bluetooth

## Software Requirements

### ESP32
- Arduino IDE or PlatformIO
- ESP32 BLE Keyboard library
- ESP32 BLE Mouse library
- Additional libraries depending on protocol (see below)

### Ubuntu
- Python 3.7+
- Required packages: `evdev`, `requests`, `websockets`, `pyserial`

## Quick Start

Choose your protocol based on your needs:

### Option 1: WebSocket Binary (Recommended)

**ESP32:**
1. Open `esp32/hid_bridge/hid_bridge_websocket.ino`
2. Install: ESP32 BLE Keyboard, ESP32 BLE Mouse, WebSocketsServer
3. Configure WiFi and upload

**Ubuntu:**
```bash
sudo python3 client/hid_client_websocket.py --esp32-ip <IP>
```

### Option 2: Serial/USB (Best Performance)

**ESP32:**
1. Open `esp32/hid_bridge/hid_bridge_serial.ino`
2. Install: ESP32 BLE Keyboard, ESP32 BLE Mouse
3. Upload to ESP32

**Ubuntu:**
```bash
sudo python3 client/hid_client_serial.py --port /dev/ttyUSB0
```

### Option 3: UDP (Ultra Low Latency)

**ESP32:**
1. Open `esp32/hid_bridge/hid_bridge_udp.ino`
2. Configure WiFi and upload

**Ubuntu:**
```bash
sudo python3 client/hid_client_udp.py --esp32-ip <IP>
```

### Option 4: HTTP/JSON (Original)

**ESP32:**
1. Open `esp32/hid_bridge/hid_bridge.ino`
2. Configure WiFi and upload

**Ubuntu:**
```bash
sudo python3 client/hid_client.py --esp32-ip <IP>
```

### Connect iPad (All Protocols)

1. Settings → Bluetooth on iPad
2. Connect to "ESP32 HID Bridge"
3. Done!

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
