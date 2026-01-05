/**
 * HID Bridge - ESP32 with Serial (USB) Communication
 *
 * Lowest possible latency using direct USB serial connection.
 * No WiFi required - ESP32 connects to Ubuntu via USB.
 *
 * Required Libraries:
 * - ESP32 BLE Keyboard (https://github.com/T-vK/ESP32-BLE-Keyboard)
 * - ESP32 BLE Mouse (https://github.com/T-vK/ESP32-BLE-Mouse)
 *
 * Binary Protocol (same as WebSocket/UDP versions):
 * Keyboard: [0x01][action:1][key:1]
 * Mouse Move: [0x02][x:1 signed][y:1 signed][wheel:1 signed]
 * Mouse Button: [0x03][action:1][button:1]
 * Status Request: [0xFE]
 * Status Response: [0xFF][keyboard_connected][mouse_connected]
 *
 * Actions: 0x01=press, 0x02=release, 0x03=click
 *
 * Advantages:
 * - Ultra-low latency (~1-2ms)
 * - No WiFi setup required
 * - More reliable than wireless
 * - No network interference
 *
 * Baud Rate: 921600 (high speed for minimal latency)
 */

#include <BleKeyboard.h>
#include <BleMouse.h>

// Serial Configuration
const unsigned long BAUD_RATE = 921600;  // High-speed serial

// BLE HID Devices
BleKeyboard bleKeyboard("ESP32 HID Bridge Serial", "HID Bridge", 100);
BleMouse bleMouse("ESP32 HID Bridge Serial", "HID Bridge", 100);

// Status LED
const int LED_PIN = 2;

// Protocol constants
const uint8_t MSG_KEYBOARD = 0x01;
const uint8_t MSG_MOUSE_MOVE = 0x02;
const uint8_t MSG_MOUSE_BUTTON = 0x03;
const uint8_t MSG_STATUS_REQUEST = 0xFE;
const uint8_t MSG_STATUS_RESPONSE = 0xFF;

const uint8_t ACTION_PRESS = 0x01;
const uint8_t ACTION_RELEASE = 0x02;
const uint8_t ACTION_CLICK = 0x03;

// Buffer for incoming serial data
uint8_t serialBuffer[256];
size_t bufferIndex = 0;

// Statistics
unsigned long messagesReceived = 0;
unsigned long lastStatsTime = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(LED_PIN, OUTPUT);

  // Wait for serial port to be ready
  delay(100);

  Serial.println("HID Bridge (USB Serial) Starting...");

  // Start Bluetooth HID devices
  Serial.println("Starting BLE Keyboard...");
  bleKeyboard.begin();

  Serial.println("Starting BLE Mouse...");
  bleMouse.begin();

  Serial.println("Serial communication ready");
  Serial.printf("Baud rate: %lu\n", BAUD_RATE);
  Serial.println("Waiting for Bluetooth connection...");
  Serial.println("---READY---");  // Marker for client to detect readiness
}

void loop() {
  // Process incoming serial data
  while (Serial.available() > 0) {
    uint8_t byte = Serial.read();

    // Check if this is a single-byte command
    if (byte == MSG_STATUS_REQUEST) {
      sendStatusResponse();
      continue;
    }

    // Store byte in buffer
    serialBuffer[bufferIndex++] = byte;

    // Process message based on type
    if (bufferIndex >= 1) {
      uint8_t msgType = serialBuffer[0];
      size_t expectedLength = getExpectedLength(msgType);

      if (bufferIndex >= expectedLength) {
        handleSerialMessage(serialBuffer, bufferIndex);
        messagesReceived++;
        bufferIndex = 0;  // Reset buffer
      }

      // Prevent buffer overflow
      if (bufferIndex >= sizeof(serialBuffer)) {
        bufferIndex = 0;
      }
    }
  }

  // Blink LED when BLE is connected
  static unsigned long lastBlink = 0;
  if (bleKeyboard.isConnected() || bleMouse.isConnected()) {
    if (millis() - lastBlink > 2000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }

  // Print statistics every 10 seconds
  if (millis() - lastStatsTime > 10000 && messagesReceived > 0) {
    Serial.printf("Messages received: %lu\n", messagesReceived);
    lastStatsTime = millis();
  }
}

size_t getExpectedLength(uint8_t msgType) {
  switch(msgType) {
    case MSG_KEYBOARD:
      return 3;  // [type][action][key]
    case MSG_MOUSE_MOVE:
      return 4;  // [type][x][y][wheel]
    case MSG_MOUSE_BUTTON:
      return 3;  // [type][action][button]
    default:
      return 1;  // Unknown, reset after one byte
  }
}

void handleSerialMessage(uint8_t* payload, size_t length) {
  if (length < 1) return;

  uint8_t msgType = payload[0];

  switch(msgType) {
    case MSG_KEYBOARD:
      if (length >= 3) {
        handleKeyboard(payload[1], payload[2]);
      }
      break;

    case MSG_MOUSE_MOVE:
      if (length >= 4) {
        int8_t x = (int8_t)payload[1];
        int8_t y = (int8_t)payload[2];
        int8_t wheel = (int8_t)payload[3];
        handleMouseMove(x, y, wheel);
      }
      break;

    case MSG_MOUSE_BUTTON:
      if (length >= 3) {
        handleMouseButton(payload[1], payload[2]);
      }
      break;
  }
}

void sendStatusResponse() {
  // Send binary status response
  uint8_t response[3] = {
    MSG_STATUS_RESPONSE,
    bleKeyboard.isConnected() ? 1 : 0,
    bleMouse.isConnected() ? 1 : 0
  };
  Serial.write(response, 3);
  Serial.flush();
}

void handleKeyboard(uint8_t action, uint8_t key) {
  if (!bleKeyboard.isConnected()) return;

  switch(action) {
    case ACTION_PRESS:
      bleKeyboard.press(key);
      break;
    case ACTION_RELEASE:
      bleKeyboard.release(key);
      break;
    case ACTION_CLICK:
      bleKeyboard.press(key);
      delay(10);
      bleKeyboard.release(key);
      break;
  }
}

void handleMouseMove(int8_t x, int8_t y, int8_t wheel) {
  if (!bleMouse.isConnected()) return;

  if (wheel != 0) {
    bleMouse.move(x, y, wheel);
  } else if (x != 0 || y != 0) {
    bleMouse.move(x, y);
  }
}

void handleMouseButton(uint8_t action, uint8_t button) {
  if (!bleMouse.isConnected()) return;

  switch(action) {
    case ACTION_PRESS:
      bleMouse.press(button);
      break;
    case ACTION_RELEASE:
      bleMouse.release(button);
      break;
    case ACTION_CLICK:
      bleMouse.click(button);
      break;
  }
}
