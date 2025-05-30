# Báo cáo dự án IoT sử dụng CoreIoT

## I. Đặt vấn đề

Trong thời đại công nghệ 4.0, việc tự động hóa và giám sát thiết bị trong nhà thông minh đang trở thành xu hướng phổ biến. Tuy nhiên, việc xây dựng một hệ thống IoT hoàn chỉnh gặp phải những thách thức sau:

- **Khó khăn trong việc tích hợp nhiều thiết bị**: Các thiết bị khác nhau cần được kết nối và điều khiển thống nhất
- **Vấn đề về kết nối mạng**: Thiết bị cần duy trì kết nối ổn định và có khả năng phục hồi khi mất kết nối
- **Yêu cầu về giao diện điều khiển**: Cần dashboard trực quan để giám sát và điều khiển từ xa
- **Tích hợp với hệ sinh thái smart home**: Khả năng tương tác với Google Home, Alexa...
- **Cập nhật firmware từ xa**: Cần có khả năng OTA (Over-The-Air) update

Dự án này nhằm xây dựng một hệ thống IoT hoàn chỉnh sử dụng ESP32 và nền tảng CoreIoT để giải quyết những vấn đề trên.

## II. Cơ sở lí thuyết

### 2.1. IoT (Internet of Things)
IoT là mạng lưới các thiết bị vật lý được tích hợp cảm biến, phần mềm và công nghệ kết nối khác nhằm kết nối và trao đổi dữ liệu với các thiết bị và hệ thống khác qua internet.

### 2.2. MQTT Protocol
Message Queuing Telemetry Transport (MQTT) là một giao thức truyền thông nhẹ, được thiết kế cho các thiết bị có băng thông thấp và kết nối không ổn định. MQTT hoạt động theo mô hình Publisher-Subscriber:
- **Publisher**: Thiết bị gửi dữ liệu
- **Subscriber**: Thiết bị nhận dữ liệu  
- **Broker**: Máy chủ trung gian điều phối việc truyền tin

### 2.3. FreeRTOS
FreeRTOS là hệ điều hành thời gian thực miễn phí cho vi điều khiển, cho phép:
- Chạy đa nhiệm (multitasking)
- Quản lý task và priority
- Đồng bộ hóa với semaphore và mutex

### 2.4. CoreIoT Platform
CoreIoT là nền tảng IoT mã nguồn mở được phát triển dựa trên ThingsBoard, cung cấp:
- Device management
- Data visualization dashboard
- Rule engine
- OTA firmware update

## III. Hệ thống, công nghệ

### 3.1. Kiến trúc hệ thống
```
[ESP32 Device] ←→ [WiFi/Internet] ←→ [CoreIoT Platform] ←→ [Dashboard]
                                   ↓
                              [SinricPro] ←→ [Google Home]
```

### 3.2. Công nghệ sử dụng

**Hardware:**
- **ESP32**: Vi điều khiển chính với WiFi/Bluetooth tích hợp
- **DHT11**: Cảm biến nhiệt độ và độ ẩm
- **WS2812B LED Strip**: Dải LED RGB có thể lập trình
- **DC Fan**: Quạt điều khiển PWM
- **Push Buttons**: Nút nhấn điều khiển thủ công

**Software:**
- **PlatformIO**: IDE phát triển
- **Arduino Framework**: Framework lập trình
- **FreeRTOS**: Hệ điều hành thời gian thực
- **ThingsBoard Client Library**: Thư viện kết nối CoreIoT
- **SinricPro Library**: Thư viện tích hợp Google Home

**Protocols & Standards:**
- **MQTT**: Giao thức truyền thông IoT
- **HTTP/HTTPS**: Web interface và API
- **WebSocket**: Real-time communication
- **JSON**: Định dạng dữ liệu

### 3.3. Luồng dữ liệu
1. **Sensor Data**: DHT11 → ESP32 → MQTT → CoreIoT → Dashboard
2. **Control Commands**: Dashboard → MQTT → ESP32 → Actuators
3. **Voice Control**: Google Home → SinricPro → ESP32 → Actuators
4. **OTA Update**: CoreIoT → MQTT → ESP32 → Firmware Flash

## IV. Thiết bị phần cứng

### 4.1. ESP32 DevKit
- **CPU**: Dual-core Tensilica LX6 @ 240MHz
- **Memory**: 520KB SRAM, 4MB Flash
- **Connectivity**: WiFi 802.11b/g/n, Bluetooth 4.2
- **GPIO**: 34 pins với ADC, DAC, PWM, I2C, SPI, UART
- **Operating Voltage**: 3.3V

### 4.2. Cảm biến DHT11
- **Tính năng**: Đo nhiệt độ và độ ẩm
- **Phạm vi nhiệt độ**: 0°C đến 50°C (±2°C)
- **Phạm vi độ ẩm**: 20% đến 90% RH (±5%)
- **Interface**: 1-Wire digital
- **Kết nối**: GPIO 6

### 4.3. WS2812B LED Strip
- **Tính năng**: LED RGB có thể lập trình cá nhân
- **Control**: 1-Wire với protocol đặc biệt
- **Voltage**: 5V
- **Kết nối**: GPIO 2

### 4.4. DC Fan với PWM Control
- **Điện áp**: 12V DC
- **Control**: PWM 25kHz, 8-bit resolution
- **Kết nối**: GPIO 10 thông qua MOSFET driver

### 4.5. Push Buttons
- **Button 0**: LED control + AP mode (long press)
- **Button 1**: Fan control
- **Type**: Pull-up với external resistor
- **Debounce**: Software debouncing

### 4.6. Sơ đồ kết nối
```
ESP32 GPIO Mapping:
├── GPIO 2:  LED Strip (WS2812B)
├── GPIO 6:  DHT11 Data Pin
├── GPIO 10: Fan PWM Control
├── GPIO 8:  Button 0 (Boot button)
└── GPIO 9:  Button 1 (External button)
```

## V. Chức năng

### 1. Fan Task

**Mô tả:**
Fan Task có trởnhiệm điều khiển quạt thông qua PWM (Pulse Width Modulation). Task này liên tục kiểm tra trạng thái của biến `fanState` và cập nhật tốc độ quạt tương ứng. Khi hệ thống ở chế độ AP Mode, task sẽ tạm dừng hoạt động để tránh xung đột tài nguyên. Quạt được điều khiển với 2 mức tốc độ: 0% (tắt) và 50% (bật) thông qua PWM channel 0 với tần số 25kHz.

**Hiện thực:**
```cpp
void fanTask(void *parameter) {
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Update fan speed based on current state
    updateFanSpeed(fanState);
    
    vTaskDelay(100 / portTICK_PERIOD_MS); // Check every 100ms
  }
}
```
**Hình kết quả:**
*(Thêm hình ảnh quạt hoạt động với các mức tốc độ khác nhau)*

### 2. Button Task

**Mô tả:**
Button Task xử lý các sự kiện từ nút nhấn vật lý, bao gồm nhấn ngắn và nhấn dài. Button 0 điều khiển LED và có thể kích hoạt AP Mode khi nhấn dài. Button 1 điều khiển quạt. Task sử dụng kỹ thuật debouncing để tránh nhiễu và đảm bảo tính chính xác. Khi có sự kiện nhấn nút, task sẽ cập nhật trạng thái thiết bị và đồng bộ với cả ThingsBoard và SinricPro.

**Hiện thực:**
```cpp
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
            lastKnownLedState = ledState; // Update tracking to prevent loops
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            
            // Only update ThingsBoard - force server state update
            if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              tb.sendAttributeData("deviceState1", ledState);
              xSemaphoreGive(tbMutex);
              Serial.printf("Button 0 pressed: LED %s (forced ThingsBoard server update)\n", ledState ? "ON" : "OFF");
            } else {
              Serial.println("ButtonTask: Failed to acquire tbMutex for LED state");
            }
            
            // Force immediate shared attribute request to trigger sync to SinricPro
            forceSharedRequest = true;
          }
          // Toggle Fan state with button 1
          else if (i == 1) {
            fanState = !fanState;
            lastKnownFanState = fanState; // Update tracking to prevent loops
            updateFanSpeed(fanState); // Use PWM control function
            
            if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              tb.sendAttributeData("deviceState2", fanState);
              xSemaphoreGive(tbMutex);
            } else {
              Serial.println("ButtonTask: Failed to acquire tbMutex for Fan state");
            }
            Serial.printf("Button 1 pressed: Fan %s (forced ThingsBoard server update)\n", fanState ? "ON" : "OFF");
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
```
**Hình kết quả:**
*(Thêm hình ảnh giao diện nút nhấn và phản hồi của hệ thống)*

### 3. MQTT Task

**Mô tả:**
MQTT Task là trái tim của hệ thống IoT, chịu trách nhiệm duy trì kết nối với CoreIoT platform thông qua giao thức MQTT. Task này xử lý việc kết nối, đăng ký các subscription cho shared attributes và RPC calls, và duy trì vòng lặp MQTT. Nó cũng quản lý việc đồng bộ hóa trạng thái thiết bị giữa dashboard, SinricPro và phần cứng, sử dụng mutex để đảm bảo thread-safety.

**Hiện thực:**
```cpp
void mqttTask(void *parameter) {
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    // Check WiFi and MQTT connection
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected. Attempting to reconnect...");
      connectWiFi(); // Try to reconnect
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait before retrying
      continue;
    }

    // Connect to ThingsBoard MQTT if not already connected
    if (!tb.connected()) {
      Serial.println("Connecting to ThingsBoard...");
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
        Serial.println("Failed to connect to ThingsBoard");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      } else {
        Serial.println("Connected to ThingsBoard");
        mqttConnected = true;
        lastKnownLedState = ledState; // Initialize with current state
        lastKnownFanState = fanState; // Initialize with current state

        // Subscribe to shared attribute updates (one-time after connection)
        if (!shared_update_subscribed) {
          Serial.println("Subscribing to shared attribute updates...");
          if (!tb.Shared_Attribute_Update_Subscribe(shared_update, processSharedAttributeUpdate)) {
            Serial.println("Failed to subscribe to shared attribute updates");
          } else {
            Serial.println("Subscribed to shared attribute updates successfully");
            shared_update_subscribed = true;
          }
        }
        
        // Subscribe to RPC requests (one-time after connection)
        if (!rpc_subscribed) {
          Serial.println("Subscribing to RPC requests...");
          // Subscribe to the RPC method for LED control
          if (!tb.RPC_Subscribe(rpc, RPC_LED_CONTROL, processLedControl)) {
            Serial.println("Failed to subscribe to RPC_LED_CONTROL");
          } else {
            Serial.println("Successfully subscribed to RPC_LED_CONTROL");
          }
          // Subscribe to the RPC method for Fan control
          if (!tb.RPC_Subscribe(rpc, RPC_FAN_CONTROL, processFanControl)) {
            Serial.println("Failed to subscribe to RPC_FAN_CONTROL");
          } else {
            Serial.println("Successfully subscribed to RPC_FAN_CONTROL");
          }
          // Subscribe to the RPC method for generic switch control
          if (!tb.RPC_Subscribe(rpc, RPC_SWITCH_METHOD, processSwitchControl)) {
            Serial.println("Failed to subscribe to RPC_SWITCH_METHOD");
          } else {
            Serial.println("Successfully subscribed to RPC_SWITCH_METHOD");
          }
          rpc_subscribed = true;
        }
      }
    }

    // Request shared attributes periodically or when forced
    if (forceSharedRequest || (millis() - lastSharedRequest > SHARED_REQUEST_INTERVAL)) {
      if (!requestedShared) {
        Serial.println("Requesting shared attributes...");
        if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          if (!tb.Shared_Attribute_Request_Send(attr_request, SHARED_ATTRIBUTES.cbegin(), SHARED_ATTRIBUTES.cend(), processSharedAttributeRequest, requestTimedOut)) {
            Serial.println("Failed to request shared attributes");
          }
          xSemaphoreGive(tbMutex);
          requestedShared = true; // Mark as requested
          lastSharedRequest = millis(); // Update timestamp
          forceSharedRequest = false; // Reset force flag
        } else {
          Serial.println("MQTT Task: Failed to acquire tbMutex for shared attributes request");
        }
      }
    } else {
      requestedShared = false; // Allow new requests if interval passed
    }
    
    // Main ThingsBoard loop
    tb.loop();

    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay for responsiveness
  }
}
```
**Hình kết quả:**
*(Thêm hình ảnh dashboard CoreIoT với dữ liệu real-time)*

### 4. Sinric Task

**Mô tả:**
Sinric Task tích hợp hệ thống với SinricPro platform để hỗ trợ điều khiển bằng giọng nói qua Google Home/Alexa. Task này duy trì kết nối với SinricPro cloud và xử lý các callback từ voice commands. Khi nhận được lệnh điều khiển từ assistant, task sẽ cập nhật trạng thái thiết bị và đồng bộ ngược lại ThingsBoard. Sử dụng mutex để đảm bảo an toàn khi truy cập SinricPro API.

**Hiện thực:**
```cpp
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
```
**Hình kết quả:**
*(Thêm hình ảnh điều khiển bằng Google Home)*

### 5. WiFi Task

**Mô tả:**
WiFi Task quản lý kết nối WiFi của thiết bị, bao gồm việc tự động kết nối lại khi mất kết nối và chuyển sang AP Mode nếu không thể kết nối sau một số lần thử. Task theo dõi trạng thái kết nối liên tục và thực hiện reconnection logic thông minh. Nếu vượt quá số lần thử kết nối tối đa (3 lần), hệ thống sẽ tự động chuyển sang AP Mode để cho phép cấu hình lại WiFi.

**Hiện thực:**
```cpp
void wifiTask(void *parameter) {
  int reconnectAttempts = 0;
  
  while (true) {
    if (!(*((bool*)parameter))) {  // If not in AP mode
      if (!wifiConnected) {
        reconnect();
        
        // If still not connected, increment the attempts counter
        if (!wifiConnected) {
          reconnectAttempts++;
          Serial.printf("Reconnect attempt %d of %d failed\n", reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
          
          // If we've reached the maximum number of attempts, enter AP mode
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            Serial.println("Maximum reconnection attempts reached. Entering AP mode.");
            setupAP();
            reconnectAttempts = 0;  // Reset counter
          }
        } else {
          // Reset the counter if we successfully connected
          reconnectAttempts = 0;
        }
      }
      
      // Check WiFi status periodically
      if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        Serial.println("WiFi connection lost, reconnecting...");
        wifiConnected = false;
      }
    } else {
      // Reset the counter when in AP mode
      reconnectAttempts = 0;
    }
    
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
}
```
**Hình kết quả:**
*(Thêm hình ảnh quá trình kết nối WiFi)*

### 6. LED Task

**Mô tả:**
LED Task điều khiển dải LED WS2812B (NeoPixel) để tạo ra các hiệu ứng ánh sáng. Khi LED được bật, task sẽ tạo hiệu ứng cầu vồng (rainbow effect) bằng cách thay đổi màu sắc liên tục. Khi tắt, tất cả LED sẽ được tắt. Task chạy với delay ngắn (50ms) để tạo ra hiệu ứng mượt mà và thu hút thị giác.

**Hiện thực:**
```cpp
void ledTask(void *parameter) {
  while (1) {
    // Update LED strip based on ledState
    updateLEDStrip(ledState);
    vTaskDelay(50 / portTICK_PERIOD_MS); // Reduced delay for smoother rainbow effect
  }
}
```
**Hình kết quả:**
*(Thêm hình ảnh LED strip với hiệu ứng rainbow)*

### 7. Sensor Task

**Mô tả:**
Sensor Task đọc dữ liệu từ cảm biến DHT11 (nhiệt độ và độ ẩm) và gửi telemetry data lên CoreIoT platform. Task thực hiện việc đọc sensor mỗi 5 giây, kiểm tra tính hợp lệ của dữ liệu, và gửi kèm theo các thông tin trạng thái khác như RSSI, device states. Dữ liệu cũng được gửi đến SinricPro để hiển thị trên Google Home. Sử dụng mutex để đảm bảo an toàn khi gửi dữ liệu.

**Hiện thực:**
```cpp
void sensorTask(void *parameter) {
  // Add DHT initialization
  dht.begin();
  Serial.println("DHT11 Sensor initialized");
  
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
    Serial.println("\n--- Reading DHT11 sensor ---");
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Small delay before reading
    
    float humidity = dht.readHumidity();
    vTaskDelay(50 / portTICK_PERIOD_MS);   // Small delay between reads
    float temperature = dht.readTemperature();
      
    // Check if any reads failed
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      Serial.print("Raw humidity value: ");
      Serial.println(humidity);
      Serial.print("Raw temperature value: ");
      Serial.println(temperature);
    } else {
      Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
        
      if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        tb.sendTelemetryData(TEMPERATURE_KEY, temperature);
        tb.sendTelemetryData(HUMIDITY_KEY, humidity);
        tb.sendAttributeData("rssi", WiFi.RSSI());
        tb.sendAttributeData("deviceState1", ledState);
        tb.sendAttributeData("deviceState2", fanState);
        xSemaphoreGive(tbMutex);
        Serial.println("Data sent to ThingsBoard successfully");
      } else {
        Serial.println("ThingsBoard: Failed to acquire mutex for telemetry");
      }
        
      // Send to SinricPro for Google Home integration
      if (wifiConnected && !apMode) {
        updateSinricProTemperature(temperature, humidity);
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
```
**Hình kết quả:**
*(Thêm hình ảnh dashboard hiển thị dữ liệu sensor)*

### 8. OTA Task

**Mô tả:**
OTA (Over-The-Air) Task cho phép cập nhật firmware từ xa thông qua CoreIoT platform. Task này gửi thông tin firmware hiện tại lên server, đăng ký nhận firmware updates, và xử lý quá trình download và flash firmware mới. Khi có update available, task sẽ tự động download và cài đặt, sau đó restart thiết bị. Đây là tính năng quan trọng cho việc bảo trì và nâng cấp hệ thống IoT.

**Hiện thực:**
```cpp
void otaTask(void *parameter) {
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
```
**Hình kết quả:**
*(Thêm hình ảnh quá trình OTA update)*

### 9. AP Mode Task

**Mô tả:**
AP Mode Task tạo một WiFi Access Point và web server để cấu hình WiFi credentials khi thiết bị không thể kết nối mạng. Task này xử lý DNS requests (captive portal) và HTTP requests từ người dùng để nhập thông tin WiFi mới. Giao diện web responsive cho phép người dùng nhập SSID và password, sau đó lưu vào EEPROM và restart thiết bị. LED sẽ nhấp nháy để báo hiệu đang ở AP Mode.

**Hiện thực:**
```cpp
void apModeTask(void *parameter) {
  while (true) {
    if (apMode) {
      dnsServer.processNextRequest();
      webServer.handleClient();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
```
**Hình kết quả:** 
*(Thêm hình ảnh giao diện web configuration)* 

## VI. Kết luận

### 6.1. Kết quả đạt được
Dự án đã thành công xây dựng một hệ thống IoT hoàn chỉnh với các tính năng chính:

- **Kết nối đa nền tảng**: Tích hợp thành công với CoreIoT và SinricPro
- **Điều khiển đa dạng**: Hỗ trợ điều khiển qua dashboard web, nút nhấn vật lý, và voice commands
- **Giám sát real-time**: Thu thập và hiển thị dữ liệu sensor liên tục
- **Tự động phục hồi**: Khả năng tự động kết nối lại và chuyển AP mode khi cần
- **OTA Update**: Cập nhật firmware từ xa một cách an toàn

### 6.2. Ưu điểm của hệ thống
- **Kiến trúc RTOS**: Sử dụng FreeRTOS cho đa nhiệm hiệu quả
- **Thread-safe**: Sử dụng mutex để đảm bảo an toàn dữ liệu
- **Fault tolerance**: Khả năng phục hồi tự động khi gặp lỗi
- **User-friendly**: Giao diện cấu hình đơn giản qua web
- **Scalable**: Dễ dàng mở rộng thêm thiết bị và chức năng

### 6.3. Hướng phát triển
- **Thêm cảm biến**: Mở rộng với các loại sensor khác (ánh sáng, chuyển động, khí gas)
- **Machine Learning**: Tích hợp AI để tự động điều chỉnh thiết bị theo thói quen người dùng
- **Mobile App**: Phát triển ứng dụng di động riêng biệt
- **Edge Computing**: Xử lý dữ liệu cục bộ để giảm độ trễ
- **Security Enhancement**: Tăng cường bảo mật với encryption và authentication

### 6.4. Ý nghĩa thực tiễn
Dự án này đóng góp vào việc nghiên cứu và phát triển các hệ thống nhà thông minh, cung cấp một solution mở có thể áp dụng trong thực tế để:
- Tiết kiệm năng lượng thông qua tự động hóa
- Nâng cao chất lượng cuộc sống
- Giảm thiểu can thiệp thủ công
- Cung cấp dữ liệu để phân tích và tối ưu hóa 
