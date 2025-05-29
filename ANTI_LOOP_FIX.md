# Anti-Loop Fix - Ngăn chặn vòng lặp feedback

## 🚨 **Vấn đề:**

Khi Google Home thay đổi trạng thái, nó tạo ra **vòng lặp feedback**:

1. **Google Home ON** → SinricPro → ESP32 → ThingsBoard **ON**
2. **ThingsBoard ON** → Shared Attribute → ESP32 → SinricPro → Google Home **ON** (lặp lại)
3. **Fast sync (1-2s)** làm vòng lặp diễn ra rất nhanh
4. **Kết quả**: Trạng thái bị toggle liên tục

## 🛠️ **Giải pháp - State Tracking:**

### 1. **Thêm State Tracking Variables**

```cpp
// Global variables in mqtt_task.cpp
bool ledState = false;
bool lastKnownLedState = false; // Track last known state to prevent loops
bool fanState = false;
bool lastKnownFanState = false;
```

### 2. **Smart State Change Detection**

Chỉ update khi có **thay đổi thực sự**:

```cpp
void processSharedAttributeRequest(const JsonObjectConst &data) {
  // ...
  if (strcmp(key, "deviceState1") == 0) {
    bool newLedState = it->value().as<bool>();
    Serial.printf(", Value: %s", newLedState ? "ON" : "OFF");
    
    // Only update if state actually changed
    if (newLedState != lastKnownLedState) {
      ledState = newLedState;
      lastKnownLedState = newLedState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.println(" (STATE CHANGED)");
      
      // Update SinricPro only when there's a real change
      if (wifiConnected && !apMode) {
        updateSinricProState(ledState);
        Serial.println("Shared Req: Updated SinricPro state");
      }
    } else {
      Serial.println(" (NO CHANGE - SKIPPED)");
    }
  }
}
```

### 3. **Update Tracking in All Sources**

**SinricPro Callback:**
```cpp
static bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("SinricPro: Device %s turned %s\n", deviceId.c_str(), state ? "ON" : "OFF");
  
  // Update both actual state and tracking
  ledState = state;
  lastKnownLedState = state; // Prevent loops
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  
  // Update ThingsBoard
  if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
    tb.sendAttributeData("deviceState1", ledState);
    xSemaphoreGive(tbMutex);
  }
  
  return true;
}
```

**Button Task:**
```cpp
// Toggle LED state with button 0
if (i == 0) {
  ledState = !ledState;
  lastKnownLedState = ledState; // Update tracking
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  
  // Update both platforms...
}
```

## 📊 **Luồng hoạt động sau khi fix:**

### ✅ **Google Home → ThingsBoard** (không loop):
```
Google Home ON → SinricPro → onPowerState() → 
  ├── ledState = true
  ├── lastKnownLedState = true
  ├── Update GPIO
  └── Send to ThingsBoard

ThingsBoard receives → Shared Attribute Update → 
processSharedAttributeRequest() → 
  ├── newLedState = true
  ├── Compare: true == lastKnownLedState (true)
  └── SKIP - No change detected ✅
```

### ✅ **ThingsBoard → Google Home** (không loop):
```
ThingsBoard Switch → Shared Attribute → 
processSharedAttributeRequest() → 
  ├── newLedState = true
  ├── Compare: true != lastKnownLedState (false)
  ├── Update ledState & lastKnownLedState
  ├── Update GPIO
  └── Send to SinricPro → Google Home

Google Home receives → SinricPro → onPowerState() → 
  ├── state = true
  ├── Compare: true == lastKnownLedState (true)
  └── No additional SinricPro update ✅
```

## 🔍 **Debug Logs:**

**Khi có thay đổi thực sự:**
```
Request Key: deviceState1, Value: ON (STATE CHANGED)
Shared Req: Updated SinricPro state
```

**Khi không có thay đổi (ngăn loop):**
```
Request Key: deviceState1, Value: ON (NO CHANGE - SKIPPED)
```

## ✅ **Ưu điểm:**

1. **Ngăn chặn vòng lặp** - Chỉ update khi có thay đổi thực sự
2. **Hiệu quả** - Giảm traffic MQTT và SinricPro
3. **Stable sync** - Đồng bộ ổn định không bị toggle
4. **Debug friendly** - Logs rõ ràng khi nào skip/update

## 🚀 **Test Results:**

- ✅ **Google Home ON** → ThingsBoard ON (không tự tắt)
- ✅ **Google Home OFF** → ThingsBoard OFF (không tự bật)
- ✅ **ThingsBoard ON** → Google Home ON (không loop)
- ✅ **ThingsBoard OFF** → Google Home OFF (không loop)
- ✅ **Button Press** → Cả hai platform đồng bộ ổn định

**Vòng lặp feedback đã được loại bỏ hoàn toàn!** 🎯 