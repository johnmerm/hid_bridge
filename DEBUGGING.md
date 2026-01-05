# Debugging the Serial Implementation

When using the serial/USB implementation, you need to choose between communication and debugging. Here are your options:

## Option 1: Use Serial2 for Communication (Best for Development)

**Hardware Setup:**
- ESP32 main USB port → Serial Monitor (debugging)
- USB-to-Serial adapter → Serial2 (GPIO16/17) → Client communication

**Wiring:**
```
USB-to-Serial Adapter     ESP32
--------------------     ------
TX        ------------>  GPIO16 (RX2)
RX        <------------  GPIO17 (TX2)
GND       ------------>  GND
```

**Firmware:** Use `hid_bridge_serial_debug.ino`

**Advantages:**
- ✅ Full debug output in Serial Monitor
- ✅ See all events in real-time
- ✅ Monitor BLE connection status
- ✅ View statistics and errors
- ✅ Best for development and troubleshooting

**Disadvantages:**
- ❌ Requires USB-to-Serial adapter (~$5-10)
- ❌ Extra wiring needed
- ❌ Two USB ports used

**Client Usage:**
```bash
# Find the USB-to-Serial adapter port
sudo python3 client/hid_client_serial.py --list-ports

# Connect using the adapter's port (not the ESP32's main port)
sudo python3 client/hid_client_serial.py --port /dev/ttyUSB1
```

**Debugging:**
```bash
# In Arduino IDE: Tools → Serial Monitor
# Or use screen/minicom:
screen /dev/ttyUSB0 115200
```

---

## Option 2: Conditional Debugging

Keep using `Serial` for both, but add a debug flag.

**Modify the firmware:**
```cpp
// Add at the top
#define DEBUG_ENABLED false  // Set to true for debugging, false for production

// Replace Serial.println() with:
#if DEBUG_ENABLED
  Serial.println("Debug message");
#endif

// Or use a macro:
#define DEBUG_PRINT(x) if(DEBUG_ENABLED) Serial.print(x)
#define DEBUG_PRINTLN(x) if(DEBUG_ENABLED) Serial.println(x)
```

**Advantages:**
- ✅ No extra hardware needed
- ✅ Single USB connection
- ✅ Easy to toggle

**Disadvantages:**
- ❌ Must recompile to enable/disable debugging
- ❌ No live debugging while running
- ❌ Debug output might interfere with binary protocol

---

## Option 3: Use Web-Based Logging

For wireless protocols, add HTTP endpoint for debug logs.

**Example (add to WebSocket/UDP versions):**
```cpp
// In setup():
server.on("/debug", HTTP_GET, []() {
  String log = getDebugLog();
  server.send(200, "text/plain", log);
});

// Access via:
// curl http://ESP32_IP/debug
```

**Advantages:**
- ✅ Remote debugging
- ✅ No extra hardware
- ✅ Works with wireless protocols

**Disadvantages:**
- ❌ Not applicable to pure Serial implementation
- ❌ Adds memory/processing overhead

---

## Option 4: LED Status Codes

Use the built-in LED for visual debugging.

**Add to your code:**
```cpp
// Error patterns
void blinkError(int code) {
  for (int i = 0; i < code; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  delay(1000);
}

// Example usage:
if (!bleKeyboard.isConnected()) {
  blinkError(2);  // 2 blinks = keyboard not connected
}
```

**Status codes:**
- 1 blink: BLE not connected
- 2 blinks: Serial communication error
- 3 blinks: Buffer overflow
- Fast blink: Processing events
- Slow blink: Normal operation

**Advantages:**
- ✅ No extra hardware
- ✅ Works while running
- ✅ Quick status at a glance

**Disadvantages:**
- ❌ Limited information
- ❌ Hard to debug complex issues

---

## Option 5: Hybrid Approach

Use different protocols for different stages.

**Development:**
1. Use WebSocket/UDP with full debugging
2. Test and fix issues
3. See all debug output over Serial Monitor

**Production:**
4. Switch to Serial for best performance
5. Remove debug code

**Advantages:**
- ✅ Best of both worlds
- ✅ Full debugging during development
- ✅ Maximum performance in production

---

## Recommended Workflow

### During Development:
```
Use: hid_bridge_serial_debug.ino
Setup: USB-to-Serial adapter on Serial2
Debug: Full output on Serial Monitor
```

### For Testing:
```
Use: hid_bridge_websocket.ino or hid_bridge_udp.ino
Debug: Serial Monitor shows all events
Test: Wireless communication
```

### For Production:
```
Use: hid_bridge_serial.ino (no debug)
Setup: Single USB connection
Performance: Maximum (1-2ms latency)
```

---

## USB-to-Serial Adapters

If you go with Option 1, you'll need an adapter:

**Recommended adapters:**
- FTDI FT232RL (~$8)
- CP2102 (~$5)
- CH340G (~$3)

**Make sure it has:**
- ✅ 3.3V logic level (or 5V with level shifter)
- ✅ TX, RX, GND pins accessible
- ✅ Linux driver support

**Testing the adapter:**
```bash
# List devices before connecting
ls /dev/ttyUSB*

# Connect adapter
# List devices after connecting (should see new device)
ls /dev/ttyUSB*

# Test communication
screen /dev/ttyUSB1 921600
```

---

## Debug Output Examples

With `hid_bridge_serial_debug.ino`, you'll see:

```
=================================
HID Bridge (USB Serial) Starting
=================================
Debug output: Serial (115200 baud)
Client comm: Serial2 (GPIO16/17, 921600 baud)

Starting BLE Keyboard...
Starting BLE Mouse...

Serial communication ready
Waiting for Bluetooth connection...
---READY---

[BLE] Keyboard CONNECTED
[BLE] Mouse CONNECTED
[KBD] Action=1 Key=4
[KBD] Action=2 Key=4
[MOUSE] Move x=10 y=-5 wheel=0
[MOUSE] Button action=1 button=1
[STATS] Messages received: 1247 (124.7 msg/sec)
```

---

## Troubleshooting

### Can't see debug output
- Check baud rate: 115200 for Serial (debug)
- Verify correct USB port for Serial Monitor
- Make sure you're using `hid_bridge_serial_debug.ino`

### Client can't connect
- Check USB-to-Serial adapter port (not ESP32 port)
- Verify RX/TX not swapped
- Try different baud rate: 115200 instead of 921600
- Check GND connection

### Debug output garbled
- Baud rate mismatch
- Electrical noise (add capacitor between 3.3V and GND)
- Poor connection

### Performance impact
- Debug output adds ~1-2ms latency
- Use conditional debugging if critical
- For production, use version without debugging
