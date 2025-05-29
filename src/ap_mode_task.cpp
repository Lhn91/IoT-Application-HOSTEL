#include "ap_mode_task.h"
#include "wifi_task.h"
#include <EEPROM.h>
#include "main_constants.h"
#include <Ticker.h>

// AP Mode configuration
const char AP_SSID[] = "ESP32_Config";
const char AP_PASSWORD[] = "12345678";
const uint16_t DNS_PORT = 53;
const uint16_t WEB_PORT = 80;
DNSServer dnsServer;
WebServer webServer(WEB_PORT);
bool apMode = false;
Ticker ledTicker; // For blinking LED in AP mode

// LED blink function for AP mode
void blinkLED() {
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
}

// EEPROM configuration constants are now in main_constants.cpp
// const int SSID_ADDR = 0;
// const int PASS_ADDR = 32;
// const int ESP_MAX_SSID_LEN = 32;
// const int ESP_MAX_PASS_LEN = 64;

void saveWiFiCredentials() {
  EEPROM.writeString(SSID_ADDR, WIFI_SSID);
  EEPROM.writeString(PASS_ADDR, WIFI_PASSWORD);
  EEPROM.commit();
  Serial.println("WiFi credentials saved to EEPROM");
}

void loadWiFiCredentials() {
  String ssid = EEPROM.readString(SSID_ADDR);
  String pass = EEPROM.readString(PASS_ADDR);
  
  if (ssid.length() > 0 && ssid.length() < ESP_MAX_SSID_LEN && pass.length() < ESP_MAX_PASS_LEN) {
    ssid.toCharArray(WIFI_SSID, ESP_MAX_SSID_LEN);
    pass.toCharArray(WIFI_PASSWORD, ESP_MAX_PASS_LEN);
    Serial.println("Loaded WiFi credentials from EEPROM:");
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("Password length: ");
    Serial.println(pass.length());
  } else {
    Serial.println("No valid WiFi credentials found in EEPROM");
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>WiFi Configuration</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial,sans-serif;margin:0;padding:20px;text-align:center;}";
  html += "h1{color:#0066cc;}";
  html += "form{display:inline-block;margin-top:20px;padding:20px;border:1px solid #ddd;border-radius:5px;}";
  html += "label{display:block;margin-bottom:5px;text-align:left;}";
  html += "input{width:100%;padding:10px;margin-bottom:15px;box-sizing:border-box;border:1px solid #ddd;border-radius:3px;}";
  html += "button{background-color:#0066cc;color:white;border:none;padding:10px 20px;border-radius:3px;cursor:pointer;}";
  html += "</style></head><body>";
  html += "<h1>ESP32 WiFi Configuration</h1>";
  html += "<form method='post' action='/save'>";
  html += "<label for='ssid'>WiFi SSID:</label>";
  html += "<input type='text' id='ssid' name='ssid' required>";
  html += "<label for='password'>WiFi Password:</label>";
  html += "<input type='password' id='password' name='password'>";
  html += "<button type='submit'>Save</button>";
  html += "</form></body></html>";
  
  webServer.send(200, "text/html", html);
}

void handleSave() {
  if (webServer.hasArg("ssid")) {
    String newSSID = webServer.arg("ssid");
    String newPassword = webServer.arg("password");
    
    if (newSSID.length() > 0 && newSSID.length() < ESP_MAX_SSID_LEN && newPassword.length() < ESP_MAX_PASS_LEN) {
      newSSID.toCharArray(WIFI_SSID, ESP_MAX_SSID_LEN);
      newPassword.toCharArray(WIFI_PASSWORD, ESP_MAX_PASS_LEN);
      
      saveWiFiCredentials();
      
      String html = "<!DOCTYPE html><html><head><title>WiFi Configuration</title>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
      html += "<style>body{font-family:Arial,sans-serif;margin:0;padding:20px;text-align:center;}";
      html += "h1{color:#0066cc;}h2{color:#4CAF50;}</style></head><body>";
      html += "<h1>WiFi Configuration Saved</h1>";
      html += "<h2>The device will now restart and attempt to connect to your WiFi network.</h2>";
      html += "</body></html>";
      
      webServer.send(200, "text/html", html);
      
      // Allow time for the response to be sent
      delay(3000);
      
      // Stop AP mode and restart
      stopAP();
      ESP.restart();
    }
  }
  
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void setupAP() {
  Serial.println("\n\n========== STARTING AP MODE ==========");
  
  // Start blinking LED to indicate AP mode
  ledTicker.attach_ms(500, blinkLED); // Blink every 500ms
  
  Serial.println("Disconnecting from existing WiFi connection...");
  WiFi.disconnect();
  
  Serial.println("Setting WiFi to AP mode...");
  WiFi.mode(WIFI_AP);
  
  Serial.print("Creating Access Point with SSID: ");
  Serial.print(AP_SSID);
  Serial.print(" and password: ");
  Serial.println(AP_PASSWORD);
  
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (apStarted) {
    Serial.println("AP started successfully!");
  } else {
    Serial.println("WARNING: Failed to start AP!");
  }
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP Mode IP address: ");
  Serial.println(IP);
  
  // Start DNS server to redirect all domains to captive portal
  Serial.println("Starting DNS server for captive portal...");
  dnsServer.start(DNS_PORT, "*", IP);
  
  // Setup web server routes
  Serial.println("Setting up web server routes...");
  webServer.on("/", handleRoot);
  webServer.on("/save", handleSave);
  webServer.onNotFound(handleRoot);
  
  Serial.println("Starting web server...");
  webServer.begin();
  
  apMode = true;
  Serial.println("AP Mode fully activated. You should now see WiFi network: " + String(AP_SSID));
  Serial.println("=======================================\n");
}

void stopAP() {
  Serial.println("\n========== STOPPING AP MODE ==========");
  
  // Stop LED blinking
  ledTicker.detach();
  digitalWrite(LED_PIN, LOW); // Turn off LED
  
  webServer.stop();
  Serial.println("Web server stopped");
  
  dnsServer.stop();
  Serial.println("DNS server stopped");
  
  WiFi.softAPdisconnect(true);
  Serial.println("WiFi AP disconnected");
  
  apMode = false;
  Serial.println("AP Mode fully deactivated");
  Serial.println("=======================================\n");
}

void apModeTask(void *parameter) {
  while (true) {
    if (apMode) {
      dnsServer.processNextRequest();
      webServer.handleClient();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
} 