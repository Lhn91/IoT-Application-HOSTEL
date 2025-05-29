#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <Arduino.h>
#include <WiFi.h>
#include "main_constants.h"

// Function declarations
void InitWiFi();
bool reconnect();
void wifiTask(void *parameter);
extern bool wifiConnected;
extern char WIFI_SSID[32];
extern char WIFI_PASSWORD[64];
extern const int WIFI_TIMEOUT;
extern const int EEPROM_SIZE;

#endif /* WIFI_TASK_H */ 