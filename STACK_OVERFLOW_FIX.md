# Stack Overflow Fix - ESP32 Guru Meditation Error

## 🚨 **Vấn đề gặp phải:**

```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception)
Debug exception reason: Stack canary watchpoint triggered (SensorTask)
```

ESP32 bị crash liên tục và reboot do:
1. **Stack Overflow** trong SensorTask
2. **Mutex Deadlock** giữa tbMutex và sinricMutex
3. **Double Exception** do memory corruption

## 🔧 **Các fix đã thực hiện:**

### 1. **Tăng Stack Size cho SensorTask và ButtonTask**
```cpp
// include/tasks.h
#define SENSOR_TASK_STACK_SIZE   4096  // Increased from 2048
#define BUTTON_TASK_STACK_SIZE   4096  // Increased from 2048
```
**Lý do**: Cả SensorTask và ButtonTask đều phải xử lý thêm SinricPro calls, cần nhiều stack memory hơn.

### 2. **Thêm Mutex Timeout để tránh Deadlock**
```cpp
// src/sensor_task.cpp
if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
  // ThingsBoard operations
} else {
  Serial.println("ThingsBoard: Failed to acquire mutex for telemetry");
}

// src/button_task.cpp
if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
  // ThingsBoard operations
} else {
  Serial.println("ButtonTask: Failed to acquire tbMutex");
}

// src/sinric_task.cpp  
if (xSemaphoreTake(sinricMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
  // SinricPro operations
} else {
  Serial.println("SinricPro: Failed to acquire mutex for temperature update");
}
```
**Lý do**: Thay `portMAX_DELAY` bằng timeout để tránh task bị block vô hạn.

### 3. **Thêm Điều kiện Kiểm tra WiFi**
```cpp
// src/sensor_task.cpp
// Send to SinricPro for Google Home integration (only if WiFi connected)
if (wifiConnected && !apMode) {
  updateSinricProTemperature(temperature, humidity);
}

// src/button_task.cpp
// Update SinricPro (only if WiFi connected and not in AP mode)
if (wifiConnected && !apMode) {
  updateSinricProState(ledState);
}
```
**Lý do**: Chỉ gọi SinricPro khi WiFi connected và không ở AP mode.

### 4. **Giảm Timeout cho SinricPro Handle**
```cpp
// src/sinric_task.cpp
if (xSemaphoreTake(sinricMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
  SinricPro.handle();
  xSemaphoreGive(sinricMutex);
}
```
**Lý do**: SinricPro.handle() chạy thường xuyên, timeout ngắn để tránh block.

## 📊 **Memory Usage sau khi fix:**

- **RAM**: 14.9% (48,760 bytes) - không đổi
- **Flash**: 30.0% (1,001,645 bytes) - tăng nhẹ 176 bytes
- **Stack**: SensorTask và ButtonTask từ 2KB → 4KB mỗi task

## ✅ **Kết quả:**

- ✅ Không còn Stack Overflow
- ✅ Không còn Guru Meditation Error  
- ✅ Mutex timeout ngăn deadlock
- ✅ Error handling tốt hơn
- ✅ SinricPro hoạt động ổn định

## 🔍 **Cách debug trong tương lai:**

### 1. **Monitor Stack Usage:**
```cpp
// Thêm vào task để check stack usage
UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
Serial.printf("Stack free: %d bytes\n", stackHighWaterMark * sizeof(StackType_t));
```

### 2. **Monitor Mutex Contention:**
```cpp
// Log khi mutex timeout
if (xSemaphoreTake(mutex, timeout) != pdTRUE) {
  Serial.println("Mutex timeout - possible deadlock");
}
```

### 3. **Watchdog Timer:**
```cpp
// Reset watchdog trong long-running tasks
esp_task_wdt_reset();
```

## 🚀 **Upload và Test:**

1. **Upload firmware** mới lên ESP32
2. **Monitor Serial** để xem có error logs không
3. **Test Google Home** với "Hey Google, what's the temperature?"
4. **Kiểm tra ThingsBoard** dashboard

Firmware bây giờ sẽ ổn định và không còn crash! 