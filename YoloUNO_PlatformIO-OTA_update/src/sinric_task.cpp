#include "sinric_task.h"
#include "wifi_task.h"
#include "ap_mode_task.h"

// Đây là file duy nhất include SinricPro
#define SINRICPRO_IMPLEMENTATION
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <SinricProTemperaturesensor.h>

// SinricPro Configuration
const char* SINRIC_APP_KEY = "d8ed1eed-cc79-4315-9028-a8509e040798";     // Replace with your SinricPro App Key
const char* SINRIC_APP_SECRET = "a3fead70-1d7f-4a9a-804b-79de1a1310bc-f860f062-8000-4312-9f25-d5653560c3c0"; // Replace with your SinricPro App Secret
const char* SWITCH_ID = "6832c78b8ed485694c3f5b3d";        // Replace with your SinricPro Device ID
const char* TEMPERATURE_SENSOR_ID = "6832de298ed485694c3f7312"; // DHT11 Temperature Sensor ID

// Create a mutex for SinricPro operations
SemaphoreHandle_t sinricMutex = NULL;

// Callback function for SinricPro switch state change
static bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("SinricPro: Device %s turned %s\n", deviceId.c_str(), state ? "ON" : "OFF");
  
  // Update the LED state and tracking variable
  ledState = state;
  lastKnownLedState = state; // Update tracking to prevent loops
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  
  // Update ThingsBoard with the new state - force server sync
  if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
    tb.sendAttributeData("deviceState1", ledState);
    xSemaphoreGive(tbMutex);
  }
  
  return true; // Indicate successful state change
}

// Function to update SinricPro with state changes from ThingsBoard or physical button
void updateSinricProState(bool state) {
  if (xSemaphoreTake(sinricMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
    mySwitch.sendPowerStateEvent(state);
    xSemaphoreGive(sinricMutex);
  }
}

// Function to update SinricPro with temperature data only
void updateSinricProTemperature(float temperature, float humidity) {
  if (xSemaphoreTake(sinricMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    SinricProTemperaturesensor& myTempSensor = SinricPro[TEMPERATURE_SENSOR_ID];
    myTempSensor.sendTemperatureEvent(temperature); // Only send temperature, not humidity
    Serial.printf("SinricPro: Sent temperature %.1f°C\n", temperature);
    xSemaphoreGive(sinricMutex);
  } else {
    Serial.println("SinricPro: Failed to acquire mutex for temperature update");
  }
}

// Initialize SinricPro
void setupSinricPro() {
  // Initialize mutex for SinricPro if not already created
  if (sinricMutex == NULL) {
    sinricMutex = xSemaphoreCreateMutex();
  }

  // Register callback function for switch device events
  SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
  mySwitch.onPowerState(onPowerState);
  
  // Setup temperature sensor (no callbacks needed for sensor-only device)
  SinricProTemperaturesensor& myTempSensor = SinricPro[TEMPERATURE_SENSOR_ID];
  
  // Setup SinricPro
  SinricPro.onConnected([]() { 
    Serial.println("Connected to SinricPro"); 
  });
  
  SinricPro.onDisconnected([]() { 
    Serial.println("Disconnected from SinricPro"); 
  });
  
  SinricPro.begin(SINRIC_APP_KEY, SINRIC_APP_SECRET);
}

// SinricPro task to be run in RTOS
void sinricTask(void *parameter) {
  setupSinricPro();
  
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Wait for WiFi connection
    if (!wifiConnected) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Handle SinricPro in a mutex-protected block
    if (xSemaphoreTake(sinricMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      SinricPro.handle();
      xSemaphoreGive(sinricMutex);
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to prevent CPU hogging
  }
} 