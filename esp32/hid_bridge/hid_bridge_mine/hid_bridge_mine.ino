// Enable/disable features (comment out to disable)
#define ENABLE_KEYBOARD
// #define ENABLE_MOUSE  // Uncomment to enable mouse support

#include <WiFi.h>
#include <WebServer.h>
#ifdef ENABLE_KEYBOARD
  #include <BleKeyboard.h>
#endif
#ifdef ENABLE_MOUSE
  #include <BleMouse.h>
#endif
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "COSMOTE-214954-2G";
const char* password = "fgddpmd7ep22f9rb";


// Server Configuration
WebServer server(80);

// BLE HID Devices
#ifdef ENABLE_KEYBOARD
  BleKeyboard bleKeyboard("ESP32 HID Bridge", "HID Bridge", 100);
#endif
#ifdef ENABLE_MOUSE
  BleMouse bleMouse("ESP32 HID Bridge", "HID Bridge", 100);
#endif

// Status LED
const int LED_PIN = 8;

const int BOOT_BUTTON_PIN = 9; // BOOT button is on GPIO 9


void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  // pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  
  Serial.println("HID Bridge Starting...");
  Serial.println("Free heap at start: " + String(ESP.getFreeHeap()));

  // Connect to WiFi FIRST (before BLE)
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

  // Start Bluetooth HID devices AFTER WiFi
  Serial.println("Free heap before BLE: " + String(ESP.getFreeHeap()));

  #ifdef ENABLE_KEYBOARD
    Serial.println("Starting BLE Keyboard...");
    bleKeyboard.begin();
    Serial.println("Free heap after keyboard: " + String(ESP.getFreeHeap()));
  #endif

  #ifdef ENABLE_MOUSE
    Serial.println("Starting BLE Mouse...");
    bleMouse.begin();
    Serial.println("Free heap after mouse: " + String(ESP.getFreeHeap()));
  #endif

  Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
  server.handleClient();

  // Blink LED when BLE is connected
  static unsigned long lastBlink = 0;
  bool bleConnected = false;
  #ifdef ENABLE_KEYBOARD
    bleConnected = bleConnected || bleKeyboard.isConnected();
  #endif
  #ifdef ENABLE_MOUSE
    bleConnected = bleConnected || bleMouse.isConnected();
  #endif

  if (bleConnected) {
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

  #ifdef ENABLE_KEYBOARD
    html += "<p>Keyboard BLE Status: " + String(bleKeyboard.isConnected() ? "Connected" : "Disconnected") + "</p>";
  #else
    html += "<p>Keyboard: DISABLED</p>";
  #endif

  #ifdef ENABLE_MOUSE
    html += "<p>Mouse BLE Status: " + String(bleMouse.isConnected() ? "Connected" : "Disconnected") + "</p>";
  #else
    html += "<p>Mouse: DISABLED</p>";
  #endif

  html += "<p>Device Name: ESP32 HID Bridge</p>";
  html += "<p>Free Heap: " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "<h2>API Endpoints:</h2>";
  html += "<ul>";
  #ifdef ENABLE_KEYBOARD
    html += "<li>POST /keyboard - Send keyboard events</li>";
  #endif
  #ifdef ENABLE_MOUSE
    html += "<li>POST /mouse - Send mouse events</li>";
  #endif
  html += "<li>GET /status - Get device status</li>";
  html += "</ul>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() {
  StaticJsonDocument<256> doc;

  bool bleConnected = false;
  #ifdef ENABLE_KEYBOARD
    doc["keyboard_connected"] = bleKeyboard.isConnected();
    bleConnected = bleConnected || bleKeyboard.isConnected();
  #else
    doc["keyboard_connected"] = false;
  #endif

  #ifdef ENABLE_MOUSE
    doc["mouse_connected"] = bleMouse.isConnected();
    bleConnected = bleConnected || bleMouse.isConnected();
  #else
    doc["mouse_connected"] = false;
  #endif

  doc["ble_connected"] = bleConnected;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.localIP().toString();
  doc["free_heap"] = ESP.getFreeHeap();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleKeyboard() {
  #ifndef ENABLE_KEYBOARD
    server.send(501, "application/json", "{\"error\":\"Keyboard support not compiled\"}");
    return;
  #endif

  #ifdef ENABLE_KEYBOARD
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
    Serial.print("Sending key ");
    uint8_t key = doc["key"];
    Serial.println(key);
    bleKeyboard.press(key);
    delay(10);
    bleKeyboard.release(key);
  } else if (strcmp(action, "down") == 0) {
    Serial.print("Down key ");
    uint8_t key = doc["key"];
    Serial.println(key);
    bleKeyboard.press(key);
  } else if (strcmp(action, "up") == 0) {
    Serial.print("Up key ");
    uint8_t key = doc["key"];
    Serial.println(key);
    bleKeyboard.release(key);
  } else if (strcmp(action, "text") == 0) {
    const char* text = doc["text"];
    bleKeyboard.print(text);
  } else if (strcmp(action, "releaseAll") == 0) {
    bleKeyboard.releaseAll();
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
  #endif
}

void handleMouse() {
  #ifndef ENABLE_MOUSE
    server.send(501, "application/json", "{\"error\":\"Mouse support not compiled\"}");
    return;
  #endif

  #ifdef ENABLE_MOUSE
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
  #endif
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
