#include "sensor_task.h"
#include "mqtt_task.h"
#include "ap_mode_task.h"
#include "wifi_task.h"
#include "sinric_task.h"

// DHT11 configuration
#define DHTPIN 6     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // Define sensor type (DHT11)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// Constants
const char TEMPERATURE_KEY[] = "temperature";
const char HUMIDITY_KEY[] = "humidity";

// External variables
extern bool ledState;
extern bool fanState;

// Sensor Reading Task
void sensorTask(void *parameter) {
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Wait for MQTT connection
    if (!mqttConnected) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
     // Read temperature and humidity from DHT11
      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();
      
      // Check if any reads failed
      if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
      } else {
        Serial.println("Sending telemetry. Temperature: " + String(temperature, 1) + " humidity: " + String(humidity, 1));
        
        if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          tb.sendTelemetryData(TEMPERATURE_KEY, temperature);
          tb.sendTelemetryData(HUMIDITY_KEY, humidity);
          tb.sendAttributeData("rssi", WiFi.RSSI()); // WiFi signal strength
          tb.sendAttributeData("deviceState1", ledState); // Current LED state
          tb.sendAttributeData("deviceState2", fanState); // Current Fan state
          xSemaphoreGive(tbMutex);
        } else {
          Serial.println("ThingsBoard: Failed to acquire mutex for telemetry");
        }
        
        // Send to SinricPro for Google Home integration (only if WiFi connected)
        if (wifiConnected && !apMode) {
          updateSinricProTemperature(temperature, humidity);
        }
      }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
} 