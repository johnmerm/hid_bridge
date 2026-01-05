# HID Bridge - Protocol Comparison

This document compares the different communication protocols available in HID Bridge.

## Available Protocols

### 1. HTTP/JSON (Original - `hid_bridge.ino`)
**Description:** RESTful API with JSON payloads

**Latency:** 10-50ms
**Overhead per event:** ~300+ bytes
**Reliability:** High (TCP)
**Complexity:** Low

**Pros:**
- Easy to debug (human-readable)
- Works with standard HTTP tools (curl, browsers)
- Simple implementation
- Good for testing

**Cons:**
- High latency
- Large packet sizes
- JSON parsing overhead
- Not ideal for real-time input

**Use case:** Development, testing, or when latency isn't critical

---

### 2. WebSocket Binary (`hid_bridge_websocket.ino`)
**Description:** Persistent WebSocket connection with binary protocol

**Latency:** 2-10ms
**Overhead per event:** ~10 bytes
**Reliability:** High (TCP)
**Complexity:** Medium

**Pros:**
- Low latency
- Minimal overhead
- Persistent connection (no handshake per message)
- Bidirectional communication
- Works through standard ports (80/81)
- Much faster than HTTP

**Cons:**
- More complex than HTTP
- Still has TCP overhead
- WebSocket handshake on connect

**Use case:** **Recommended for most users** - best balance of performance and reliability

---

### 3. UDP Binary (`hid_bridge_udp.ino`)
**Description:** Connectionless UDP with binary protocol

**Latency:** 1-5ms
**Overhead per event:** ~5 bytes
**Reliability:** None (fire-and-forget)
**Complexity:** Medium

**Pros:**
- Ultra-low latency
- Minimal overhead
- No connection handshake
- Perfect for mouse movements
- Lowest CPU usage

**Cons:**
- No delivery guarantee
- Packets can be lost or arrive out of order
- May need firewall configuration
- Lost keyboard events = stuck keys

**Use case:** When absolute minimum latency is required and occasional packet loss is acceptable (e.g., competitive gaming)

---

### 4. Serial/USB (`hid_bridge_serial.ino`)
**Description:** Direct USB serial communication

**Latency:** 1-2ms
**Overhead per event:** ~3-4 bytes
**Reliability:** Very High
**Complexity:** Low

**Pros:**
- **Lowest possible latency**
- No WiFi configuration needed
- Most reliable connection
- No wireless interference
- Simple setup
- No network overhead

**Cons:**
- Requires USB cable connection
- ESP32 must be near computer
- Less portable than wireless options
- Uses a USB port

**Use case:** **Best for lowest latency** - ideal when you need the absolute best performance and don't need wireless

---

## Binary Protocol Specification

All binary protocols use the same message format:

### Keyboard Events
```
[0x01][action][key]
```
- `action`: 0x01=press, 0x02=release, 0x03=click
- `key`: HID keycode (0-255)

**Size:** 3 bytes

### Mouse Movement
```
[0x02][x][y][wheel]
```
- `x`: signed int8 (-127 to 127)
- `y`: signed int8 (-127 to 127)
- `wheel`: signed int8 (-127 to 127)

**Size:** 4 bytes

### Mouse Button
```
[0x03][action][button]
```
- `action`: 0x01=press, 0x02=release, 0x03=click
- `button`: 1=left, 2=right, 4=middle

**Size:** 3 bytes

### Status Request (client → ESP32)
```
[0xFE]
```
**Size:** 1 byte

### Status Response (ESP32 → client)
```
[0xFF][keyboard_connected][mouse_connected]
```
- `keyboard_connected`: 0=no, 1=yes
- `mouse_connected`: 0=no, 1=yes

**Size:** 3 bytes

---

## Performance Comparison

| Protocol | Latency | Bandwidth | Packet Loss | Setup Complexity |
|----------|---------|-----------|-------------|------------------|
| HTTP/JSON | 10-50ms | High | None | Low |
| WebSocket | 2-10ms | Low | None | Medium |
| UDP | 1-5ms | Minimal | Possible | Medium |
| Serial | 1-2ms | Minimal | None | Low |

---

## Choosing the Right Protocol

### For Development/Testing
→ **HTTP/JSON** - Easy to debug and understand

### For General Use
→ **WebSocket Binary** - Great balance of performance and reliability

### For Competitive Gaming
→ **UDP Binary** - Absolute minimum latency, can tolerate occasional packet loss

### For Best Performance
→ **Serial/USB** - Lowest latency, most reliable, no wireless issues

### For Wireless Freedom
→ **WebSocket Binary** or **UDP Binary** - No cables needed

---

## Client Usage

### HTTP/JSON
```bash
sudo python3 hid_client.py --esp32-ip 192.168.1.100
```

### WebSocket Binary
```bash
sudo python3 hid_client_websocket.py --esp32-ip 192.168.1.100
```

### UDP Binary
```bash
sudo python3 hid_client_udp.py --esp32-ip 192.168.1.100
```

### Serial/USB
```bash
# List available ports
sudo python3 hid_client_serial.py --list-ports

# Connect to ESP32
sudo python3 hid_client_serial.py --port /dev/ttyUSB0
```

---

## Required Libraries by Protocol

### HTTP/JSON
- ESP32: WebServer, ArduinoJson, BleKeyboard, BleMouse
- Python: requests, evdev

### WebSocket Binary
- ESP32: WebSocketsServer (by Markus Sattler), BleKeyboard, BleMouse
- Python: websockets, evdev

### UDP Binary
- ESP32: WiFiUdp, BleKeyboard, BleMouse
- Python: evdev (socket is built-in)

### Serial/USB
- ESP32: BleKeyboard, BleMouse
- Python: pyserial, evdev

---

## Benchmarks (Typical Values)

**Mouse Movement Latency** (Ubuntu → iPad):
- HTTP/JSON: 25ms average
- WebSocket: 6ms average
- UDP: 3ms average
- Serial: 1.5ms average

**Keyboard Latency** (Ubuntu → iPad):
- HTTP/JSON: 20ms average
- WebSocket: 5ms average
- UDP: 3ms average (risk of lost events)
- Serial: 1ms average

**Throughput** (mouse movements per second):
- HTTP/JSON: ~50-100 events/sec
- WebSocket: ~500-1000 events/sec
- UDP: ~1000-2000 events/sec
- Serial: ~2000+ events/sec

*Note: Actual performance varies based on WiFi quality, CPU load, and distance.*
