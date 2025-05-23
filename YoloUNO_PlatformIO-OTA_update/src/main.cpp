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
#include <Server_Side_RPC.h>
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
constexpr uint8_t MAX_ATTRIBUTES = 5U; //
constexpr std::array<const char*, MAX_ATTRIBUTES> 
SHARED_ATTRIBUTES = 
{
  "deviceState1",
  "deviceState2",
  "deviceState3",
  "deviceState4",
  "deviceState5"
};

// RPC Methods
constexpr const char RPC_LED_CONTROL[] = "set_led";
constexpr const char RPC_FAN_CONTROL[] = "set_fan";
constexpr const char RPC_SWITCH_METHOD[] = "set_switch";
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;

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

// GPIO Pins for control
#define LED_PIN 2  // LED pin
#define FAN_PIN 3  // Fan pin
bool ledState = false;
bool fanState = false;

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
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation*, 4U> apis = {
    &shared_update,
    &attr_request,
    &ota,
    &rpc
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
bool rpc_subscribed = false;

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

// LED Control
void processLedControl(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received LED control RPC command");

  if (data.containsKey("value")) {
    bool led_value = data["value"];
    ledState = led_value;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.printf("Setting LED to: %s\n", ledState ? "ON" : "OFF");
    
    // Send attribute update to ThingsBoard
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      tb.sendAttributeData("deviceState1", ledState);
      xSemaphoreGive(tbMutex);
    }
    
    response["status"] = "success";
    response["value"] = ledState;
  } else {
    response["status"] = "error";
    response["error"] = "No value provided";
  }
}

// Fan Control
void processFanControl(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received Fan control RPC command");

  if (data.containsKey("value")) {
    bool fan_value = data["value"];
    fanState = fan_value;
    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
    Serial.printf("Setting Fan to: %s\n", fanState ? "ON" : "OFF");
    
    // Send attribute update to ThingsBoard
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      tb.sendAttributeData("deviceState2", fanState);
      xSemaphoreGive(tbMutex);
    }
    
    response["status"] = "success";
    response["value"] = fanState;
  } else {
    response["status"] = "error";
    response["error"] = "No value provided";
  }
}

// Generic Switch Control
void processSwitchControl(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received Switch control RPC command");

  if (data.containsKey("switch")) {
    bool switch_value = data["switch"];
    // Switch can control either LED or Fan based on context
    // For this example, we'll control the LED
    ledState = switch_value;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.printf("Setting Switch (LED) to: %s\n", ledState ? "ON" : "OFF");
    
    // Send attribute update to ThingsBoard
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      tb.sendAttributeData("deviceState1", ledState);
      xSemaphoreGive(tbMutex);
    }
    
    response["status"] = "success";
    response["value"] = ledState;
  } else {
    response["status"] = "error";
    response["error"] = "No switch value provided";
  }
}

void processSharedAttributeUpdate(const JsonObjectConst &data) {
  Serial.println("Received shared attribute update:");
  
  for (auto it = data.begin(); it != data.end(); ++it) {
    const char* key = it->key().c_str();
    Serial.print("Key: ");
    Serial.print(key);
    
    // Handle different attribute types
    if (strcmp(key, "deviceState1") == 0) {
      ledState = it->value().as<bool>();
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.printf(", Value: %s\n", ledState ? "ON" : "OFF");
    } 
    else if (strcmp(key, "deviceState2") == 0) {
      fanState = it->value().as<bool>();
      digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
      Serial.printf(", Value: %s\n", fanState ? "ON" : "OFF");
    }
    else if (strcmp(key, "deviceState3") == 0) {
      bool state3 = it->value().as<bool>();
      // Implement device 3 control logic
      Serial.printf(", Value: %s\n", state3 ? "ON" : "OFF");
    }
    else if (strcmp(key, "deviceState4") == 0) {
      bool state4 = it->value().as<bool>();
      // Implement device 4 control logic
      Serial.printf(", Value: %s\n", state4 ? "ON" : "OFF");
    }
    else if (strcmp(key, "deviceState5") == 0) {
      bool state5 = it->value().as<bool>();
      // Implement device 5 control logic
      Serial.printf(", Value: %s\n", state5 ? "ON" : "OFF");
    }
    else {
      Serial.println(", Value type not handled");
    }
  }

  // Debug print the whole JSON
  const size_t jsonSize = Helper::Measure_Json(data);
  char buffer[jsonSize];
  serializeJson(data, buffer, jsonSize);
  Serial.println(buffer);
}

void processSharedAttributeRequest(const JsonObjectConst &data) {
  Serial.println("Received shared attribute request:");
  
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
        
        if (!rpc_subscribed) {
          Serial.println("Subscribing for RPC...");
          const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
            RPC_Callback{ RPC_LED_CONTROL, processLedControl },
            RPC_Callback{ RPC_FAN_CONTROL, processFanControl },
            RPC_Callback{ RPC_SWITCH_METHOD, processSwitchControl }
          };
          
          if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
            Serial.println("Failed to subscribe for RPC");
          } else {
            Serial.println("RPC Subscribe done");
            rpc_subscribed = true;
          }
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
          tb.sendAttributeData("deviceState1", ledState); // Current LED state
          tb.sendAttributeData("deviceState2", fanState); // Current Fan state
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
          // Toggle LED state with button 0
          if (i == 0) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
              tb.sendAttributeData("deviceState1", ledState);
              xSemaphoreGive(tbMutex);
            }
            Serial.printf("Button 0 pressed: LED %s\n", ledState ? "ON" : "OFF");
          }
          // Toggle Fan state with button 1
          else if (i == 1) {
            fanState = !fanState;
            digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
            if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
              tb.sendAttributeData("deviceState2", fanState);
              xSemaphoreGive(tbMutex);
            }
            Serial.printf("Button 1 pressed: Fan %s\n", fanState ? "ON" : "OFF");
          }
        }
        if (isButtonLongPressed(i)) {
          Serial.printf("Button %d pressed (long)\n", i);
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Initialize serial connection for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  Serial.println("RTOTA2");
  delay(1000);
  
  // Initialize GPIO pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  
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