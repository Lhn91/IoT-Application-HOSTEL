#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include <Arduino.h>
#include <ThingsBoard.h>
#include "button.h"
#include "Ticker.h"
#include "main_constants.h"

// Button task configuration
extern Ticker buttonTicker;
extern const int buttonPins[NUM_BUTTONS];

// External variables from mqtt_task.cpp
extern bool ledState;
extern bool lastKnownLedState;
extern bool fanState;
extern bool forceSharedRequest;
extern SemaphoreHandle_t tbMutex;
extern ThingsBoard tb;

// Function declarations
void buttonTask();
void ButtonTask(void *pvParameters);

#endif /* BUTTON_TASK_H */ 