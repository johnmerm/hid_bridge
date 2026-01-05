/**
 * HID Bridge - ESP32 Bluetooth HID Device (Keyboard + Mouse)
 *
 * Receives keyboard and mouse events via WiFi and forwards them
 * as Bluetooth HID reports to connected devices (iPad, etc.)
 *
 * Required Libraries:
 * - ESP32 BLE Keyboard (https://github.com/T-vK/ESP32-BLE-Keyboard)
 * - ESP32 BLE Mouse (https://github.com/T-vK/ESP32-BLE-Mouse)
 * - ArduinoJson (version 6.x)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <BleKeyboard.h>
#include <BleMouse.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server Configuration
WebServer server(80);

// BLE HID Devices
BleKeyboard bleKeyboard("ESP32 HID Bridge", "HID Bridge", 100);
BleMouse bleMouse("ESP32 HID Bridge", "HID Bridge", 100);

// Status LED
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("HID Bridge Starting...");

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

  // Setup HTTP server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/keyboard", HTTP_POST, handleKeyboard);
  server.on("/mouse", HTTP_POST, handleMouse);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
  server.handleClient();

  // Blink LED when BLE is connected
  static unsigned long lastBlink = 0;
  if (bleKeyboard.isConnected() || bleMouse.isConnected()) {
    if (millis() - lastBlink > 2000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>HID Bridge</h1>";
  html += "<p>ESP32 IP: " + WiFi.localIP().toString() + "</p>";
  html += "<p>Keyboard BLE Status: " + String(bleKeyboard.isConnected() ? "Connected" : "Disconnected") + "</p>";
  html += "<p>Mouse BLE Status: " + String(bleMouse.isConnected() ? "Connected" : "Disconnected") + "</p>";
  html += "<p>Device Name: ESP32 HID Bridge</p>";
  html += "<h2>API Endpoints:</h2>";
  html += "<ul>";
  html += "<li>POST /keyboard - Send keyboard events</li>";
  html += "<li>POST /mouse - Send mouse events</li>";
  html += "<li>GET /status - Get device status</li>";
  html += "</ul>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() {
  StaticJsonDocument<256> doc;
  doc["ble_connected"] = bleKeyboard.isConnected() || bleMouse.isConnected();
  doc["keyboard_connected"] = bleKeyboard.isConnected();
  doc["mouse_connected"] = bleMouse.isConnected();
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.localIP().toString();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleKeyboard() {
  if (!bleKeyboard.isConnected()) {
    server.send(503, "application/json", "{\"error\":\"BLE not connected\"}");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  const char* action = doc["action"];

  if (strcmp(action, "press") == 0) {
    uint8_t key = doc["key"];
    bleKeyboard.press(key);
    delay(10);
    bleKeyboard.release(key);
  } else if (strcmp(action, "down") == 0) {
    uint8_t key = doc["key"];
    bleKeyboard.press(key);
  } else if (strcmp(action, "up") == 0) {
    uint8_t key = doc["key"];
    bleKeyboard.release(key);
  } else if (strcmp(action, "text") == 0) {
    const char* text = doc["text"];
    bleKeyboard.print(text);
  } else if (strcmp(action, "releaseAll") == 0) {
    bleKeyboard.releaseAll();
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleMouse() {
  if (!bleMouse.isConnected()) {
    server.send(503, "application/json", "{\"error\":\"BLE Mouse not connected\"}");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  const char* action = doc["action"];

  if (strcmp(action, "move") == 0) {
    int8_t x = doc["x"] | 0;
    int8_t y = doc["y"] | 0;
    int8_t wheel = doc["wheel"] | 0;

    if (wheel != 0) {
      bleMouse.move(x, y, wheel);
    } else {
      bleMouse.move(x, y);
    }
  } else if (strcmp(action, "click") == 0) {
    uint8_t button = doc["button"] | MOUSE_LEFT;
    bleMouse.click(button);
  } else if (strcmp(action, "press") == 0) {
    uint8_t button = doc["button"] | MOUSE_LEFT;
    bleMouse.press(button);
  } else if (strcmp(action, "release") == 0) {
    uint8_t button = doc["button"] | MOUSE_LEFT;
    bleMouse.release(button);
  } else if (strcmp(action, "releaseAll") == 0) {
    bleMouse.release(MOUSE_LEFT);
    bleMouse.release(MOUSE_RIGHT);
    bleMouse.release(MOUSE_MIDDLE);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
