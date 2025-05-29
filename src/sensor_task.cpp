#include "sensor_task.h"
#include "mqtt_task.h"
#include "ap_mode_task.h"
#include "wifi_task.h"
#include "sinric_task.h"
#include "DHT20.h"
// DHT11 configuration
#define DHTPIN 6     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // Define sensor type (DHT11)
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
DHT20 dht20; // Initialize DHT20 sensor (if needed)
// Constants
const char HUMIDITY_KEY[] = "humidity";
const char Indoor_TEMPERATURE_KEY[] = "tempIndoor";
const char Outdoor_TEMPERATURE_KEY[] = "tempOutdoor";
// External variables
extern bool ledState;
extern bool fanState;

// Sensor Reading Task
void sensorTask(void *parameter) {
  // Khởi tạo DHT20 với các chân SDA và SCL
  Wire.begin(SDA_PIN, SCL_PIN);
  dht20.begin();  // Khởi tạo cảm biến DHT20
  
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
     // Read temperature and humidity from DHT11 (indoor)
      float humidity = dht.readHumidity();
      float tempIndoor = dht.readTemperature();
      
      // Read temperature from DHT20 (outdoor)
      float tempOutdoor = NAN;
      if (dht20.read() == DHT20_OK) {
        tempOutdoor = dht20.getTemperature();
        Serial.println("DHT20 read success: " + String(tempOutdoor, 1) + "°C");
      } else {
        Serial.println("Failed to read from DHT20 sensor!");
      }
      
      // Check if any reads failed
      if (isnan(humidity) || isnan(tempIndoor)) {
        Serial.println("Failed to read from DHT11 sensor!");
      } else {
        Serial.println("Indoor Temperature: " + String(tempIndoor, 1) + 
                       "°C, Outdoor Temperature: " + String(tempOutdoor, 1) + 
                       "°C, Humidity: " + String(humidity, 1) + "%");
        
        if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          // Gửi nhiệt độ trong nhà (DHT11)
          tb.sendTelemetryData(Indoor_TEMPERATURE_KEY, tempIndoor);
          
          // Gửi nhiệt độ ngoài trời (DHT20) nếu đọc thành công
          if (!isnan(tempOutdoor)) {
            tb.sendTelemetryData(Outdoor_TEMPERATURE_KEY, tempOutdoor);
          }
          
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
          updateSinricProTemperature(tempIndoor, humidity);
        }
      }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}