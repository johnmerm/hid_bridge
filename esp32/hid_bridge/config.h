/**
 * Configuration for HID Bridge
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Device Configuration
#define DEVICE_NAME "ESP32 HID Bridge"
#define DEVICE_MANUFACTURER "HID Bridge Project"
#define BATTERY_LEVEL 100

// Server Configuration
#define SERVER_PORT 80

// LED Pin
#define STATUS_LED_PIN 2

// Timeouts (milliseconds)
#define WIFI_CONNECT_TIMEOUT 30000
#define BLE_CONNECT_TIMEOUT 60000

#endif
