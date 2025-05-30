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
#define FAN_PIN 10  // Fan pin (GPIO 10)
#define AP_MODE_PIN 0  // Button pin for forcing AP mode

// PWM configuration for fan
#define FAN_PWM_CHANNEL 0  // PWM channel for fan
#define FAN_PWM_FREQ 25000 // PWM frequency in Hz
#define FAN_PWM_RESOLUTION 8 // 8-bit resolution (0-255)
#define FAN_PWM_SPEED_50 127 // 50% duty cycle (255 * 0.5)
#define FAN_PWM_SPEED_0 0    // 0% duty cycle

#endif /* MAIN_CONSTANTS_H */ 