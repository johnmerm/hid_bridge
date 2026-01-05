/**
 * HID Bridge - ESP32 with UDP Binary Protocol
 *
 * Ultra-low latency version using UDP with binary protocol.
 * UDP is connectionless and has minimal overhead - ideal for mouse movement.
 * Note: No delivery guarantee, but acceptable for HID events.
 *
 * Required Libraries:
 * - ESP32 BLE Keyboard (https://github.com/T-vK/ESP32-BLE-Keyboard)
 * - ESP32 BLE Mouse (https://github.com/T-vK/ESP32-BLE-Mouse)
 *
 * Binary Protocol (same as WebSocket version):
 * Keyboard: [0x01][action:1][key:1]
 * Mouse Move: [0x02][x:1 signed][y:1 signed][wheel:1 signed]
 * Mouse Button: [0x03][action:1][button:1]
 *
 * Actions: 0x01=press, 0x02=release, 0x03=click
 *
 * Default UDP Port: 8888
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// UDP Configuration
WiFiUDP udp;
const unsigned int udpPort = 8888;
uint8_t udpBuffer[256];

// BLE HID Devices
BleKeyboard bleKeyboard("ESP32 HID Bridge UDP", "HID Bridge", 100);
BleMouse bleMouse("ESP32 HID Bridge UDP", "HID Bridge", 100);

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

// Statistics
unsigned long packetsReceived = 0;
unsigned long lastStatsTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("HID Bridge (UDP Binary) Starting...");

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
    return;
  }

  // Start UDP
  if (udp.begin(udpPort)) {
    Serial.printf("UDP server started on port %d\n", udpPort);
  } else {
    Serial.println("Failed to start UDP server!");
  }

  Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
  // Check for UDP packets
  int packetSize = udp.parsePacket();
  if (packetSize) {
    packetsReceived++;

    // Read the packet
    int len = udp.read(udpBuffer, sizeof(udpBuffer));
    if (len > 0) {
      handleUDPMessage(udpBuffer, len);

      // Handle status requests
      if (len > 0 && udpBuffer[0] == MSG_STATUS_REQUEST) {
        sendStatusResponse();
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
  if (millis() - lastStatsTime > 10000) {
    Serial.printf("Packets received: %lu\n", packetsReceived);
    lastStatsTime = millis();
  }
}

void handleUDPMessage(uint8_t* payload, size_t length) {
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

    case MSG_STATUS_REQUEST:
      // Status response sent in main loop
      break;
  }
}

void sendStatusResponse() {
  uint8_t response[4] = {
    MSG_STATUS_RESPONSE,
    bleKeyboard.isConnected() ? 1 : 0,
    bleMouse.isConnected() ? 1 : 0,
    WiFi.status() == WL_CONNECTED ? 1 : 0
  };

  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(response, 4);
  udp.endPacket();
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
