/**
 * HID Bridge - ESP32 with Serial (USB) Communication + Debugging
 *
 * Uses Serial2 for client communication while Serial remains available for debugging.
 *
 * Pinout:
 * - Serial (RX0/TX0): USB debugging via Serial Monitor
 * - Serial2 (RX2=GPIO16, TX2=GPIO17): Client communication
 *
 * Connect your USB-to-Serial adapter to:
 * - ESP32 GPIO16 (RX2) → Adapter TX
 * - ESP32 GPIO17 (TX2) → Adapter RX
 * - ESP32 GND → Adapter GND
 *
 * OR use the main USB port for communication and disable debugging:
 * Just change all Serial2 references back to Serial in the code.
 *
 * Required Libraries:
 * - ESP32 BLE Keyboard (https://github.com/T-vK/ESP32-BLE-Keyboard)
 * - ESP32 BLE Mouse (https://github.com/T-vK/ESP32-BLE-Mouse)
 *
 * Binary Protocol (same as other versions):
 * Keyboard: [0x01][action:1][key:1]
 * Mouse Move: [0x02][x:1 signed][y:1 signed][wheel:1 signed]
 * Mouse Button: [0x03][action:1][button:1]
 *
 * Baud Rate: 921600 (high speed for minimal latency)
 */

#include <BleKeyboard.h>
#include <BleMouse.h>

// Serial Configuration
const unsigned long BAUD_RATE = 921600;

// Serial2 pins (can be changed if needed)
const int RX2_PIN = 16;  // GPIO16
const int TX2_PIN = 17;  // GPIO17

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
  // Serial for debugging
  Serial.begin(115200);
  delay(100);

  // Serial2 for client communication
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX2_PIN, TX2_PIN);

  pinMode(LED_PIN, OUTPUT);

  Serial.println("=================================");
  Serial.println("HID Bridge (USB Serial) Starting");
  Serial.println("=================================");
  Serial.printf("Debug output: Serial (115200 baud)\n");
  Serial.printf("Client comm: Serial2 (GPIO16/17, %lu baud)\n", BAUD_RATE);

  // Start Bluetooth HID devices
  Serial.println("\nStarting BLE Keyboard...");
  bleKeyboard.begin();

  Serial.println("Starting BLE Mouse...");
  bleMouse.begin();

  Serial.println("\nSerial communication ready");
  Serial.println("Waiting for Bluetooth connection...");
  Serial.println("---READY---");

  // Send ready signal to client
  Serial2.println("---READY---");
}

void loop() {
  // Process incoming serial data from client
  while (Serial2.available() > 0) {
    uint8_t byte = Serial2.read();

    // Check if this is a single-byte command
    if (byte == MSG_STATUS_REQUEST) {
      sendStatusResponse();
      Serial.println("[DEBUG] Status request received");
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
        Serial.println("[ERROR] Buffer overflow, resetting");
        bufferIndex = 0;
      }
    }
  }

  // Blink LED when BLE is connected
  static unsigned long lastBlink = 0;
  static bool lastKeyboardState = false;
  static bool lastMouseState = false;

  bool keyboardConnected = bleKeyboard.isConnected();
  bool mouseConnected = bleMouse.isConnected();

  // Log connection state changes
  if (keyboardConnected != lastKeyboardState) {
    Serial.printf("[BLE] Keyboard %s\n", keyboardConnected ? "CONNECTED" : "DISCONNECTED");
    lastKeyboardState = keyboardConnected;
  }
  if (mouseConnected != lastMouseState) {
    Serial.printf("[BLE] Mouse %s\n", mouseConnected ? "CONNECTED" : "DISCONNECTED");
    lastMouseState = mouseConnected;
  }

  if (keyboardConnected || mouseConnected) {
    if (millis() - lastBlink > 2000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }

  // Print statistics every 10 seconds
  if (millis() - lastStatsTime > 10000 && messagesReceived > 0) {
    Serial.printf("[STATS] Messages received: %lu (%.1f msg/sec)\n",
                  messagesReceived, messagesReceived / 10.0);
    messagesReceived = 0;
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
        Serial.printf("[KBD] Action=%d Key=%d\n", payload[1], payload[2]);
        handleKeyboard(payload[1], payload[2]);
      }
      break;

    case MSG_MOUSE_MOVE:
      if (length >= 4) {
        int8_t x = (int8_t)payload[1];
        int8_t y = (int8_t)payload[2];
        int8_t wheel = (int8_t)payload[3];
        // Only log non-zero movements to reduce spam
        if (x != 0 || y != 0 || wheel != 0) {
          Serial.printf("[MOUSE] Move x=%d y=%d wheel=%d\n", x, y, wheel);
        }
        handleMouseMove(x, y, wheel);
      }
      break;

    case MSG_MOUSE_BUTTON:
      if (length >= 3) {
        Serial.printf("[MOUSE] Button action=%d button=%d\n", payload[1], payload[2]);
        handleMouseButton(payload[1], payload[2]);
      }
      break;

    default:
      Serial.printf("[WARN] Unknown message type: 0x%02X\n", msgType);
      break;
  }
}

void sendStatusResponse() {
  // Send binary status response to client
  uint8_t response[3] = {
    MSG_STATUS_RESPONSE,
    bleKeyboard.isConnected() ? 1 : 0,
    bleMouse.isConnected() ? 1 : 0
  };
  Serial2.write(response, 3);
  Serial2.flush();
}

void handleKeyboard(uint8_t action, uint8_t key) {
  if (!bleKeyboard.isConnected()) {
    Serial.println("[WARN] Keyboard not connected");
    return;
  }

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
  if (!bleMouse.isConnected()) {
    return;  // Don't spam warning for every movement
  }

  if (wheel != 0) {
    bleMouse.move(x, y, wheel);
  } else if (x != 0 || y != 0) {
    bleMouse.move(x, y);
  }
}

void handleMouseButton(uint8_t action, uint8_t button) {
  if (!bleMouse.isConnected()) {
    Serial.println("[WARN] Mouse not connected");
    return;
  }

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
