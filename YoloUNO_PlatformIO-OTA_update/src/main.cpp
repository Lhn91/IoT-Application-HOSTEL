#define CONFIG_THINGSBOARD_ENABLE_DEBUG false
#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <OTA_Firmware_Update.h>
#include <ThingsBoard.h>
#include <Shared_Attribute_Update.h>
#include <Attribute_Request.h>
#include <Espressif_Updater.h>
#include <DHT.h>
#include <DHT_U.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "button.h"
#include "Ticker.h"
// Task handles
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;
Ticker buttonTicker;
volatile bool state = false;
// Semaphores for resource protection
SemaphoreHandle_t tbMutex = NULL;

//Shared Attributes Configuration
constexpr uint8_t MAX_ATTRIBUTES = 2U; //
constexpr std::array<const char*, MAX_ATTRIBUTES> 
SHARED_ATTRIBUTES = 
{
  "POWER",
  "ledState"
};

constexpr int16_t TELEMETRY_SEND_INTERVAL = 5000U;
uint32_t previousTelemetrySend; 
// Firmware title and version used to compare with remote version, to check if an update is needed.
// Title needs to be the same and version needs to be different --> downgrading is possible
constexpr char CURRENT_FIRMWARE_TITLE[] = "RTOTA";
constexpr char CURRENT_FIRMWARE_VERSION[] = "2";
// Maximum amount of retries we attempt to download each firmware chunck over MQTT
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
// Size of each firmware chunck downloaded over MQTT,
// increased packet size, might increase download speed
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;

constexpr char WIFI_SSID[] = "HCMUT09";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr char TOKEN[] = "wKVmVLxdNixrgQkwzEup";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr char TEMPERATURE_KEY[] = "temperature";
constexpr char HUMIDITY_KEY[] = "humidity";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 512U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 512U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 10000U * 1000U;

// DHT11 configuration
#define DHTPIN 6     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // Define sensor type (DHT11)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// Task configurations
#define WIFI_TASK_STACK_SIZE     4096
#define MQTT_TASK_STACK_SIZE     8192
#define SENSOR_TASK_STACK_SIZE   2048
#define OTA_TASK_STACK_SIZE      8192
#define BUTTON_TASK_STACK_SIZE   2048
#define WIFI_TASK_PRIORITY       1
#define MQTT_TASK_PRIORITY       1
#define SENSOR_TASK_PRIORITY     1
#define OTA_TASK_PRIORITY        1
#define BUTTON_TASK_PRIORITY     1

void requestTimedOut() {
  Serial.printf("Attribute request timed out did not receive a response in (%llu) microseconds. Ensure client is connected to the MQTT broker and that the keys actually exist on the target device\n", REQUEST_TIMEOUT_MICROSECONDS);
}

// Initialize underlying client, used to establish a connection
WiFiClient espClient;
// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);
// Initialize used apis
OTA_Firmware_Update<> ota;
Shared_Attribute_Update<1U, MAX_ATTRIBUTES> shared_update;
Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
const std::array<IAPI_Implementation*, 3U> apis = {
    &shared_update,
    &attr_request,
    &ota
};
// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);
// Initalize the Updater client instance used to flash binary to flash memory
Espressif_Updater<> updater;

// Statuses for updating
bool shared_update_subscribed = false;
bool currentFWSent = false;
bool updateRequestSent = false;
bool requestedShared = false;
bool wifiConnected = false;
bool mqttConnected = false;

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    // Delay 500ms until a connection has been successfully established
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
  wifiConnected = true;
}

bool reconnect() {
  // Check to ensure we aren't connected yet
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifiConnected = true;
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return true;
}

void update_starting_callback() {
}

void finished_callback(const bool & success) {
  if (success) {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t & current, const size_t & total) {
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}

void processSharedAttributeUpdate(const JsonObjectConst &data) {
  //Info
  const size_t jsonSize = Helper::Measure_Json(data);
  char buffer[jsonSize];
  serializeJson(data, buffer, jsonSize);
  Serial.println(buffer);
}

void processSharedAttributeRequest(const JsonObjectConst &data) {
  //Info
  const size_t jsonSize = Helper::Measure_Json(data);
  char buffer[jsonSize];
  serializeJson(data, buffer, jsonSize);
  Serial.println(buffer);
}

// WiFi Connection Task
void wifiTask(void *parameter) {
  while (true) {
    if (!wifiConnected) {
      reconnect();
    }
    
    // Check WiFi status periodically
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost, reconnecting...");
      wifiConnected = false;
      reconnect();
    }
    
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
}

// MQTT and ThingsBoard Communication Task
void mqttTask(void *parameter) {
  while (true) {
    // Wait for WiFi connection
    if (!wifiConnected) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      if (!tb.connected()) {
        // Reconnect to the ThingsBoard server,
        // if a connection was disrupted or has not yet been established
        Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
        mqttConnected = tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
        if (!mqttConnected) {
          Serial.println("Failed to connect to MQTT");
          xSemaphoreGive(tbMutex);
          vTaskDelay(5000 / portTICK_PERIOD_MS); // Retry after 5 seconds
          continue;
        }
        
        if (!requestedShared) {
          Serial.println("Requesting shared attributes...");
          const Attribute_Request_Callback<MAX_ATTRIBUTES> sharedCallback(&processSharedAttributeRequest, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut, SHARED_ATTRIBUTES);
          requestedShared = attr_request.Shared_Attributes_Request(sharedCallback);
          if (!requestedShared) {
            Serial.println("Failed to request shared attributes");
          }
        }

        if (!shared_update_subscribed) {
          Serial.println("Subscribing for shared attribute updates...");
          const Shared_Attribute_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeUpdate, SHARED_ATTRIBUTES);
          if (!shared_update.Shared_Attributes_Subscribe(callback)) {
            Serial.println("Failed to subscribe for shared attribute updates");
          }
          Serial.println("Subscribe done");
          shared_update_subscribed = true;
        }
      }
      
      // Process MQTT messages
      tb.loop();
      xSemaphoreGive(tbMutex);
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS); // Process MQTT frequently
  }
}

// Sensor Reading Task
void sensorTask(void *parameter) {
  //uint32_t lastTelemetrySend = 0;
  
  while (true) {
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
        
        if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
          tb.sendTelemetryData(TEMPERATURE_KEY, temperature);
          tb.sendTelemetryData(HUMIDITY_KEY, humidity);
          tb.sendAttributeData("rssi", WiFi.RSSI()); // WiFi signal strength
          xSemaphoreGive(tbMutex);
        }
      }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// OTA Update Task
void otaTask(void *parameter) {
  while (true) {
    // Wait for MQTT connection
    if (!mqttConnected) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      if (!currentFWSent) {
        currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
      }
      
      if (!updateRequestSent) {
        Serial.print(CURRENT_FIRMWARE_TITLE);
        Serial.println(CURRENT_FIRMWARE_VERSION);
        Serial.println("Firmware Update ...");
        const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = ota.Start_Firmware_Update(callback);
        if (updateRequestSent) {
          delay(500);
          Serial.println("Firmware Update Subscription...");
          updateRequestSent = ota.Subscribe_Firmware_Update(callback);
        }
      }
      
      xSemaphoreGive(tbMutex);
    }
    
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check OTA less frequently
  }
}

void buttonTask() {
    getKeyInput();
}
void ButtonTask(void *pvParameters) {
while (1) {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck >= 100) { // Kiểm tra mỗi 100ms
    lastCheck = millis();
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (isButtonPressed(i)) {
        state = !state;
        Serial.printf("State: %d \n", state);
        Serial.printf("Button %d pressed (short)\n", i);
      }
      if (isButtonLongPressed(i)) {
        Serial.printf("Button %d pressed (long)\n", i);
      }
    }
  }
}
}

void setup() {
  // Initialize serial connection for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  Serial.println("RTOTA2");
  delay(1000);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Create mutex for ThingsBoard operations
  tbMutex = xSemaphoreCreateMutex();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
buttonTicker.attach_ms(10, buttonTask);
  // Create RTOS tasks
  xTaskCreate(
    wifiTask,          // Task function
    "WiFiTask",        // Name
    WIFI_TASK_STACK_SIZE,  // Stack size
    NULL,              // Parameters
    WIFI_TASK_PRIORITY,    // Priority
    &wifiTaskHandle    // Task handle
  );
  
  xTaskCreate(
    mqttTask,
    "MQTTTask",
    MQTT_TASK_STACK_SIZE,
    NULL,
    MQTT_TASK_PRIORITY,
    &mqttTaskHandle
  );
  
  xTaskCreate(
    sensorTask,
    "SensorTask",
    SENSOR_TASK_STACK_SIZE,
    NULL,
    SENSOR_TASK_PRIORITY,
    &sensorTaskHandle
  );
  
  xTaskCreate(
    otaTask,
    "OTATask",
    OTA_TASK_STACK_SIZE,
    NULL,
    OTA_TASK_PRIORITY,
    &otaTaskHandle
  );
  xTaskCreate(
    ButtonTask,
    "ButtonTask",
    BUTTON_TASK_STACK_SIZE,
    NULL,
    BUTTON_TASK_PRIORITY,
    &buttonTaskHandle
  );
  // Note: The scheduler starts automatically after setup()
}

void loop() {
  // Empty as functionality is moved to RTOS tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}