#ifndef AP_MODE_TASK_H
#define AP_MODE_TASK_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "main_constants.h"

// AP Mode configuration
extern const char AP_SSID[];
extern const char AP_PASSWORD[];
extern const uint16_t DNS_PORT;
extern const uint16_t WEB_PORT;
extern DNSServer dnsServer;
extern WebServer webServer;
extern bool apMode;
extern const int EEPROM_SIZE;

// Function declarations
void setupAP();
void stopAP();
void handleRoot();
void handleSave();
void apModeTask(void *parameter);
void saveWiFiCredentials();
void loadWiFiCredentials();

#endif /* AP_MODE_TASK_H */ 