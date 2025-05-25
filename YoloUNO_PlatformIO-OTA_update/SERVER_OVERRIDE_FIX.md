# Server Override Fix - Ngăn chặn ThingsBoard server override device state

## 🚨 **Vấn đề:**

ThingsBoard server đang **override device state** với giá trị cũ:

1. **Dashboard ON** → Server lưu shared attribute = `true`
2. **Google Home OFF** → Device state = `false`, nhưng server vẫn = `true`
3. **Shared attribute request** → Server trả về `true` → **Override device thành ON**
4. **Button OFF** → Device state = `false`, nhưng server vẫn = `true`
5. **Shared attribute request** → Server trả về `true` → **Override device thành ON**

## 🛠️ **Giải pháp - Device State Authority:**

### 1. **Device làm chủ state, không để server override**

Thay vì chấp nhận server state, device sẽ **correct server state**:

```cpp
// OLD - Chấp nhận server override
void processSharedAttributeRequest(const JsonObjectConst &data) {
  if (newLedState != lastKnownLedState) {
    ledState = newLedState;  // Chấp nhận server state
    lastKnownLedState = newLedState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    updateSinricProState(ledState);
  }
}

// NEW - Device làm chủ, correct server
void processSharedAttributeRequest(const JsonObjectConst &data) {
  if (newLedState != lastKnownLedState) {
    Serial.printf("SERVER OVERRIDE DETECTED - Server wants %s but device is %s", 
                 newLedState ? "ON" : "OFF", 
                 lastKnownLedState ? "ON" : "OFF");
    
    // Correct server with current device state
    tb.sendAttributeData("deviceState1", lastKnownLedState);
    Serial.printf("CORRECTED server to %s", lastKnownLedState ? "ON" : "OFF");
  }
}
```

### 2. **Force Server Updates**

Khi có thay đổi từ SinricPro hoặc Button, **force update server**:

```cpp
// SinricPro callback
static bool onPowerState(const String &deviceId, bool &state) {
  ledState = state;
  lastKnownLedState = state;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  
  // Force server sync
  tb.sendAttributeData("deviceState1", ledState);
  Serial.printf("SinricPro: Forced ThingsBoard server update to %s", ledState ? "ON" : "OFF");
  
  return true;
}

// Button task
if (i == 0) {
  ledState = !ledState;
  lastKnownLedState = ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  
  // Force server state update
  tb.sendAttributeData("deviceState1", ledState);
  Serial.printf("Button: forced ThingsBoard server update to %s", ledState ? "ON" : "OFF");
}
```

### 3. **Reduce Override Frequency**

Giảm shared request interval để tránh override quá thường xuyên:

```cpp
const unsigned long SHARED_REQUEST_INTERVAL = 5000; // 5 seconds to avoid conflicts
```

## 📊 **Luồng hoạt động sau khi fix:**

### ✅ **Google Home OFF** (không bị override):
```
Google Home OFF → SinricPro → onPowerState() → 
  ├── ledState = false
  ├── lastKnownLedState = false
  ├── Update GPIO
  └── FORCE server update to false

Shared Attribute Request (5s later) → 
  ├── Server value: true (cũ)
  ├── Device state: false (hiện tại)
  ├── DETECT override conflict
  └── CORRECT server to false ✅
```

### ✅ **Button OFF** (không bị override):
```
Button Press → 
  ├── ledState = false
  ├── lastKnownLedState = false
  ├── Update GPIO
  └── FORCE server update to false

Shared Attribute Request (5s later) → 
  ├── Server value: false (đã được correct)
  ├── Device state: false
  └── STATES MATCH - OK ✅
```

### ✅ **Dashboard ON** (legitimate change):
```
Dashboard Switch → Shared Attribute Update → 
  ├── Update value: true
  ├── Device state: false
  ├── DASHBOARD CHANGE ACCEPTED
  ├── Update device to true
  └── Sync to SinricPro ✅
```

## 🔍 **Debug Logs:**

**Server Override Detection:**
```
Request Key: deviceState1, Server Value: ON, Current State: OFF (SERVER OVERRIDE DETECTED - Server wants ON but device is OFF) - CORRECTED server to OFF
```

**Legitimate Dashboard Change:**
```
Update Key: deviceState1, Update Value: ON, Current State: OFF (DASHBOARD CHANGE ACCEPTED)
Shared Attr: Updated SinricPro state to ON
```

**States Match:**
```
Request Key: deviceState1, Server Value: OFF, Current State: OFF (STATES MATCH - OK)
```

**Force Server Updates:**
```
SinricPro: Forced ThingsBoard server update to OFF
Button: forced ThingsBoard server update to ON
```

## ✅ **Kết quả:**

- ✅ **Google Home changes**: Không bị server override
- ✅ **Button changes**: Không bị server override  
- ✅ **Dashboard changes**: Vẫn hoạt động bình thường
- ✅ **Server state**: Luôn sync với device state
- ✅ **No conflicts**: Device làm chủ state authority

## 🚀 **Test Scenario:**

1. **Dashboard ON** → LED ON
2. **Google Home OFF** → LED OFF (không bật lại)
3. **Button ON** → LED ON (không tắt lại)
4. **Dashboard OFF** → LED OFF
5. **All platforms sync** ổn định

**Server override đã được loại bỏ hoàn toàn!** 🎯 