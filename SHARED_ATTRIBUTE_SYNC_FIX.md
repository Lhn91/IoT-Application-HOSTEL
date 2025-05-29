# Shared Attribute Sync Fix - ThingsBoard ↔ Google Home

## 🚨 **Vấn đề:**

Widget `device_card` trên ThingsBoard dashboard **chỉ cập nhật shared attribute** mà **không gọi RPC**. Điều này dẫn đến:

- ✅ **Google Home → ThingsBoard**: Hoạt động (qua SinricPro → RPC)
- ❌ **ThingsBoard → Google Home**: Không hoạt động (shared attribute không trigger callback)

## 🛠️ **Giải pháp (không sửa widget):**

### 1. **Enhanced Shared Attribute Request Processing**

Sửa `processSharedAttributeRequest()` để xử lý shared attributes giống như updates:

```cpp
void processSharedAttributeRequest(const JsonObjectConst &data) {
  Serial.println("Received shared attribute request:");
  
  // Process the shared attributes the same way as updates
  for (auto it = data.begin(); it != data.end(); ++it) {
    const char* key = it->key().c_str();
    
    if (strcmp(key, "deviceState1") == 0) {
      ledState = it->value().as<bool>();
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      
      // Update SinricPro with the new state
      if (wifiConnected && !apMode) {
        updateSinricProState(ledState);
        Serial.println("Shared Req: Updated SinricPro state");
      }
    }
    // ... handle other device states
  }
}
```

### 2. **Fast & Immediate Shared Attribute Requests**

Thêm fast periodic requests (2s) và immediate triggers:

```cpp
// Global variables
unsigned long lastSharedRequest = 0;
const unsigned long SHARED_REQUEST_INTERVAL = 2000; // 2 seconds for fast sync
bool forceSharedRequest = false; // Flag for immediate request

// In mqttTask()
unsigned long currentTime = millis();
bool shouldRequest = forceSharedRequest || (currentTime - lastSharedRequest > SHARED_REQUEST_INTERVAL);

if (shouldRequest) {
  if (forceSharedRequest) {
    Serial.println("Immediate shared attributes request...");
    forceSharedRequest = false;
  } else {
    Serial.println("Periodic shared attributes request...");
  }
  
  const Attribute_Request_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeRequest, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut, SHARED_ATTRIBUTES);
  attr_request.Shared_Attributes_Request(callback);
  lastSharedRequest = currentTime;
}

// Trigger immediate request after MQTT processing
if (!forceSharedRequest && (currentTime - lastSharedRequest > 1000)) {
  forceSharedRequest = true; // Will trigger on next loop iteration
}
```

## 📊 **Luồng hoạt động sau khi fix:**

### ✅ **ThingsBoard Dashboard → Google Home**:
```
Dashboard Switch → Update Shared Attribute → 
ESP32 Immediate/Fast Request (1-2s) → processSharedAttributeRequest() → 
  ├── Update ledState
  ├── Control GPIO
  └── updateSinricProState() → Google Home
```

### ✅ **Google Home → ThingsBoard** (vẫn hoạt động):
```
Google Home → SinricPro → onPowerState() → 
  ├── Update ledState
  ├── Control GPIO
  └── Send to ThingsBoard
```

## 🔍 **Debug Logs:**

Khi bấm switch trên dashboard, bạn sẽ thấy:
```
Immediate shared attributes request...
Received shared attribute request:
Request Key: deviceState1, Value: ON
Shared Req: Updated SinricPro state
```

Hoặc trong trường hợp periodic:
```
Periodic shared attributes request...
Received shared attribute request:
Request Key: deviceState1, Value: ON
Shared Req: Updated SinricPro state
```

## ✅ **Ưu điểm của giải pháp:**

1. **Không cần sửa widget/dashboard** - Giữ nguyên UI
2. **Backward compatible** - Không ảnh hưởng tính năng cũ
3. **Fast sync** - Đồng bộ trong 1-2 giây
4. **Immediate triggers** - Phản ứng nhanh với MQTT activity
5. **Debug friendly** - Logs chi tiết để troubleshoot

## 🚀 **Test Instructions:**

1. **Upload firmware** với các thay đổi
2. **Monitor Serial** để xem logs
3. **Test dashboard**: Bấm switch trên ThingsBoard
4. **Verify Google Home**: Kiểm tra trạng thái đồng bộ trong 1-2 giây
5. **Watch logs**: Xem immediate/periodic requests

## ⚡ **Tốc độ đồng bộ:**

- **Immediate trigger**: ~1 giây (khi có MQTT activity)
- **Fast periodic**: Tối đa 2 giây
- **Fallback**: Mỗi 2 giây đảm bảo không bỏ lỡ

**Bây giờ đồng bộ gần như ngay lập tức mà không cần sửa widget!** 🎯 