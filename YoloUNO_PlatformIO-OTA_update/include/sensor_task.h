#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>

// DHT11 configuration
extern DHT dht;

// Constants
extern const char TEMPERATURE_KEY[];
extern const char HUMIDITY_KEY[];

// Function declarations
void sensorTask(void *parameter);

#endif /* SENSOR_TASK_H */ 