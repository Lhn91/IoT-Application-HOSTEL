#ifndef OTA_TASK_H
#define OTA_TASK_H

#include <Arduino.h>
#include <OTA_Firmware_Update.h>
#include <Espressif_Updater.h>
#include "mqtt_task.h"

// OTA configuration
extern const char CURRENT_FIRMWARE_TITLE[];
extern const char CURRENT_FIRMWARE_VERSION[];
extern const uint8_t FIRMWARE_FAILURE_RETRIES;
extern const uint16_t FIRMWARE_PACKET_SIZE;

// Global variables
extern Espressif_Updater<> updater;
extern bool currentFWSent;
extern bool updateRequestSent;

// Function declarations
void update_starting_callback();
void finished_callback(const bool &success);
void progress_callback(const size_t &current, const size_t &total);
void otaTask(void *parameter);

#endif /* OTA_TASK_H */ 