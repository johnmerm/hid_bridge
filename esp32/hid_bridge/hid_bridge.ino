/**
 * HID Bridge - ESP32 Bluetooth HID Device
 *
 * Receives keyboard and mouse events via WiFi and forwards them
 * as Bluetooth HID reports to connected devices (iPad, etc.)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <BleKeyboard.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server Configuration
WebServer server(80);

// BLE HID Device
BleKeyboard bleKeyboard("ESP32 HID Bridge", "HID Bridge", 100);

// Status LED
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("HID Bridge Starting...");

  // Start Bluetooth HID
  Serial.println("Starting BLE HID...");
  bleKeyboard.begin();

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
  if (bleKeyboard.isConnected()) {
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
  html += "<p>BLE Status: " + String(bleKeyboard.isConnected() ? "Connected" : "Disconnected") + "</p>";
  html += "<p>Device Name: ESP32 HID Bridge</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() {
  StaticJsonDocument<200> doc;
  doc["ble_connected"] = bleKeyboard.isConnected();
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
    bleKeyboard.write(key);
  } else if (strcmp(action, "pressRaw") == 0) {
    uint8_t key = doc["key"];
    uint8_t modifier = doc["modifier"] | 0;
    if (modifier) {
      // Handle modifier keys
    }
    bleKeyboard.write(key);
  } else if (strcmp(action, "text") == 0) {
    const char* text = doc["text"];
    bleKeyboard.print(text);
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleMouse() {
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

  // Note: BleKeyboard library doesn't support mouse by default
  // You'll need to use BleMouse library or implement custom HID descriptors
  // This is a placeholder for now

  server.send(501, "application/json", "{\"error\":\"Mouse not implemented yet\"}");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
