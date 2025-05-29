#ifndef SINRIC_TASK_H
#define SINRIC_TASK_H

#include <Arduino.h>
#include "mqtt_task.h"

// SinricPro Configuration
extern const char* SINRIC_APP_KEY;
extern const char* SINRIC_APP_SECRET;
extern const char* SWITCH_ID;
extern const char* TEMPERATURE_SENSOR_ID;

// Function declarations
void setupSinricPro();
void sinricTask(void *parameter);
void updateSinricProState(bool state);
void updateSinricProTemperature(float temperature, float humidity); // Only temperature sent to SinricPro

// External variables from mqtt_task.cpp
extern bool ledState;
extern bool lastKnownLedState;
extern SemaphoreHandle_t tbMutex;
extern SemaphoreHandle_t sinricMutex;

#endif /* SINRIC_TASK_H */ 