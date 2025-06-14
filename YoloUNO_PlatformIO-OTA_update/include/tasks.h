#ifndef TASKS_H
#define TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Task handles
extern TaskHandle_t wifiTaskHandle;
extern TaskHandle_t mqttTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t otaTaskHandle;
extern TaskHandle_t buttonTaskHandle;
extern TaskHandle_t apModeTaskHandle;
extern TaskHandle_t sinricTaskHandle;
extern TaskHandle_t energyTaskHandle;

// Semaphore cho các thao tác ThingsBoard
extern SemaphoreHandle_t tbMutex;

// Định nghĩa stack size và priority cho các task
#define WIFI_TASK_STACK_SIZE     4096
#define MQTT_TASK_STACK_SIZE     8192
#define SENSOR_TASK_STACK_SIZE   4096
#define OTA_TASK_STACK_SIZE      8192
#define BUTTON_TASK_STACK_SIZE   4096  // Increased for SinricPro integration
#define AP_MODE_TASK_STACK_SIZE  4096
#define SINRIC_TASK_STACK_SIZE   4096
#define ENERGY_TASK_STACK_SIZE   6144  // Larger stack for RFID and energy calculations
#define WIFI_TASK_PRIORITY       1
#define MQTT_TASK_PRIORITY       1
#define SENSOR_TASK_PRIORITY     1
#define OTA_TASK_PRIORITY        1
#define BUTTON_TASK_PRIORITY     1
#define AP_MODE_TASK_PRIORITY    1
#define SINRIC_TASK_PRIORITY     1
#define ENERGY_TASK_PRIORITY     2  // Higher priority for real-time energy monitoring

// Hàm khởi tạo semaphore
void initSemaphores();

// Hàm khởi tạo tất cả các task
void createAllTasks();

#endif /* TASKS_H */ 