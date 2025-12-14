# Ubuntu HID Client

Captures keyboard and mouse events on Ubuntu and forwards them to ESP32.

## Installation

```bash
# Install Python dependencies
pip install -r requirements.txt

# Or install system-wide
sudo pip3 install evdev requests
```

## Usage

### Basic Usage

```bash
sudo python3 hid_client.py --esp32-ip 192.168.1.100
```

### Specify Devices Manually

```bash
# List available input devices
sudo python3 -c "from evdev import list_devices, InputDevice; [print(f'{d}: {InputDevice(d).name}') for d in list_devices()]"

# Use specific devices
sudo python3 hid_client.py --esp32-ip 192.168.1.100 \
    --keyboard /dev/input/event3 \
    --mouse /dev/input/event4
```

## Requirements

- Python 3.7+
- Root privileges (needed to access `/dev/input/*` devices)
- evdev library (for input capture)
- requests library (for HTTP communication)

## How It Works

1. **Input Capture**: Uses `evdev` to read raw input events from `/dev/input/*`
2. **Event Processing**: Translates Linux input events to HID reports
3. **HTTP Transmission**: Sends events to ESP32 via HTTP POST
4. **Auto-detection**: Automatically finds keyboard and mouse devices

## Supported Events

### Keyboard
- Key press/release
- All standard keys
- Modifier keys (Ctrl, Alt, Shift, etc.)

### Mouse
- Movement (X, Y)
- Scroll wheel
- Left/Right/Middle click
- Press and release events

## Configuration

No configuration file needed - all settings via command-line arguments.

## Troubleshooting

### Permission Denied

```bash
# Must run as root
sudo python3 hid_client.py --esp32-ip <IP>
```

### Device Not Found

```bash
# Check available devices
ls -la /dev/input/

# List devices with names
sudo evtest
```

### High CPU Usage

The client uses async I/O for efficiency, but if you experience high CPU:
- Increase the movement send interval in code (currently 0.01s)
- Reduce event sampling rate

### Connection Errors

- Ensure ESP32 is on the same network
- Check ESP32 IP address in serial monitor
- Test with: `curl http://<ESP32_IP>/status`

## Advanced Usage

### Run as Systemd Service

Create `/etc/systemd/system/hid-bridge.service`:

```ini
[Unit]
Description=HID Bridge Client
After=network.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 /path/to/hid_client.py --esp32-ip 192.168.1.100
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable hid-bridge
sudo systemctl start hid-bridge
```

### Keyboard Layout Mapping

The client sends raw key codes. If your keyboard layout differs between Ubuntu and iPad:
- Modify the key code mapping in `hid_client.py`
- Or change iPad keyboard layout to match

## Performance

- **Latency**: ~10-50ms depending on WiFi
- **CPU Usage**: <5% on modern systems
- **Network**: ~1-10 KB/s depending on input activity
