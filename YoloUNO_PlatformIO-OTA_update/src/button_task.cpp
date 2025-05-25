#include "button_task.h"
#include "mqtt_task.h"
#include "ap_mode_task.h"
#include "Ticker.h"

// Button task configuration
Ticker buttonTicker;

void buttonTask() {
    getKeyInput();
}

void ButtonTask(void *pvParameters) {
  while (1) {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck >= 100) { // Check every 100ms
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
          // Long press button 0 to enter AP mode
          if (i == 0 && !apMode) {
            Serial.println("Long press detected. Entering AP mode for configuration...");
            setupAP();
          }
          Serial.printf("Button %d pressed (long)\n", i);
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
} 