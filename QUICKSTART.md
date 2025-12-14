# Quick Start Guide

Get up and running in 20 minutes!

## ⚡ TL;DR

1. **Flash ESP32** with `esp32/hid_bridge/hid_bridge_combo.ino`
2. **Configure WiFi** credentials in the code
3. **Note ESP32 IP** from serial monitor
4. **Pair iPad** with "ESP32 HID Bridge" via Bluetooth
5. **Run client**: `sudo python3 client/hid_client.py --esp32-ip <IP>`

---

## 📋 Prerequisites

- ESP32 board + USB cable
- Ubuntu laptop
- iPad
- Arduino IDE installed
- Python 3.7+ installed

---

## 🔧 ESP32 Setup (10 min)

### 1. Install Libraries in Arduino IDE

Go to: **Sketch → Include Library → Manage Libraries**

Install:
- `ESP32-BLE-Combo` by blackketter
- `ArduinoJson` by Benoit Blanchon

### 2. Configure & Upload

```cpp
// Edit in hid_bridge_combo.ino:
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";
```

**Board**: Tools → Board → ESP32 Dev Module
**Port**: Tools → Port → (select your ESP32)
**Upload**: Click → button

### 3. Get IP Address

Open Serial Monitor (115200 baud):
```
WiFi connected!
IP address: 192.168.1.XXX  ← Note this!
```

---

## 📱 iPad Setup (2 min)

1. Settings → Bluetooth
2. Connect to **"ESP32 HID Bridge"**
3. Done! (no PIN needed)

---

## 💻 Ubuntu Setup (5 min)

```bash
cd client
pip install -r requirements.txt

# Test connection (replace with your ESP32 IP)
sudo python3 test_connection.py --esp32-ip 192.168.1.XXX
```

---

## 🚀 Run It!

```bash
sudo python3 client/hid_client.py --esp32-ip 192.168.1.XXX
```

Now type on your Ubuntu laptop - it appears on your iPad! 🎉

---

## 🔍 Troubleshooting

| Problem | Solution |
|---------|----------|
| WiFi won't connect | Check SSID/password, use 2.4GHz WiFi |
| BLE won't pair | Reset ESP32, forget device on iPad |
| Permission denied | Run with `sudo` |
| Mouse not working | Use `hid_bridge_combo.ino` not `hid_bridge.ino` |

Full troubleshooting: See [SETUP.md](SETUP.md)

---

## 📖 API Reference

### Keyboard Events

```bash
# POST /keyboard
curl -X POST http://192.168.1.XXX/keyboard \
  -H "Content-Type: application/json" \
  -d '{"action":"text","text":"Hello!"}'
```

### Mouse Events

```bash
# POST /mouse
curl -X POST http://192.168.1.XXX/mouse \
  -H "Content-Type: application/json" \
  -d '{"action":"move","x":10,"y":10}'
```

### Check Status

```bash
# GET /status
curl http://192.168.1.XXX/status
```

---

## 📚 More Info

- [Complete Setup Guide](SETUP.md)
- [README](README.md)
- [ESP32 Documentation](esp32/README.md)
- [Client Documentation](client/README.md)

---

## 🎯 What's Next?

- ✅ Test keyboard input
- ✅ Test mouse movement
- ✅ Test clicks and scrolling
- 📦 Add battery to ESP32 for portability
- 🔄 Set up auto-start on boot
- 🎨 Customize key mappings
