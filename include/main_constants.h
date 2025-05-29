#ifndef MAIN_CONSTANTS_H
#define MAIN_CONSTANTS_H

// EEPROM configuration
extern const int EEPROM_SIZE;
extern const int SSID_ADDR;
extern const int PASS_ADDR;
extern const int ESP_MAX_SSID_LEN;
extern const int ESP_MAX_PASS_LEN;

// GPIO Pins for control
#define LED_PIN 2  // LED pin
#define FAN_PIN 3  // Fan pin
#define AP_MODE_PIN 0  // Button pin for forcing AP mode

#endif /* MAIN_CONSTANTS_H */ 