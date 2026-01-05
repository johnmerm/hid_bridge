/**
 * HID Bridge - ESP32 with WebSocket Binary Protocol
 *
 * High-performance version using WebSocket with binary protocol
 * for minimal latency and overhead.
 *
 * Required Libraries:
 * - ESP32 BLE Keyboard (https://github.com/T-vK/ESP32-BLE-Keyboard)
 * - ESP32 BLE Mouse (https://github.com/T-vK/ESP32-BLE-Mouse)
 * - WebSockets by Markus Sattler (https://github.com/Links2004/arduinoWebSockets)
 *
 * Binary Protocol:
 * Keyboard: [0x01][action:1][key:1]
 * Mouse Move: [0x02][x:1 signed][y:1 signed][wheel:1 signed]
 * Mouse Button: [0x03][action:1][button:1]
 *
 * Actions: 0x01=press, 0x02=release, 0x03=click
 */

#include <WiFi.h>
#include <WebSocketsServer.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// WebSocket Server
WebSocketsServer webSocket = WebSocketsServer(81);

// BLE HID Devices
BleKeyboard bleKeyboard("ESP32 HID Bridge WS", "HID Bridge", 100);
BleMouse bleMouse("ESP32 HID Bridge WS", "HID Bridge", 100);

// Status LED
const int LED_PIN = 2;

// Protocol constants
const uint8_t MSG_KEYBOARD = 0x01;
const uint8_t MSG_MOUSE_MOVE = 0x02;
const uint8_t MSG_MOUSE_BUTTON = 0x03;

const uint8_t ACTION_PRESS = 0x01;
const uint8_t ACTION_RELEASE = 0x02;
const uint8_t ACTION_CLICK = 0x03;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("HID Bridge (WebSocket Binary) Starting...");

  // Start Bluetooth HID devices
  Serial.println("Starting BLE Keyboard...");
  bleKeyboard.begin();

  Serial.println("Starting BLE Mouse...");
  bleMouse.begin();

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("\nWiFi connection failed!");
    digitalWrite(LED_PIN, LOW);
  }

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");
  Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
  webSocket.loop();

  // Blink LED when BLE is connected
  static unsigned long lastBlink = 0;
  if (bleKeyboard.isConnected() || bleMouse.isConnected()) {
    if (millis() - lastBlink > 2000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);

      // Send status message
      uint8_t status[3] = {
        0xFF,  // Status message type
        bleKeyboard.isConnected() ? 1 : 0,
        bleMouse.isConnected() ? 1 : 0
      };
      webSocket.sendBIN(num, status, 3);
      break;
    }

    case WStype_BIN:
      handleBinaryMessage(payload, length);
      break;

    case WStype_TEXT:
      Serial.println("Text messages not supported, use binary");
      break;
  }
}

void handleBinaryMessage(uint8_t* payload, size_t length) {
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
