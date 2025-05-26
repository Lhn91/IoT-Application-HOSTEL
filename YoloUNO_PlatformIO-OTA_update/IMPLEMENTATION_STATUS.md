# Energy Management System - Implementation Status

## ✅ Đã Hoàn thành

### 1. Core Architecture
- **RTOS Task Structure**: Energy management task đã được tích hợp vào hệ thống RTOS
- **RFID Integration**: Hỗ trợ đầy đủ cho RFID RC522 với SPI interface
- **Session Management**: Quản lý phiên sử dụng của nhiều user đồng thời
- **Power Measurement**: Tích hợp M5 Stack AC Measure Unit qua I2C
- **Energy Calculation**: Tính toán điện năng tiêu thụ real-time
- **ThingsBoard Integration**: Gửi telemetry và session data lên cloud

### 2. Hardware Support
- **RFID RC522**: Pin configuration và SPI communication
- **M5 Stack AC Measure**: I2C communication protocol
- **NTP Time Sync**: Đồng bộ thời gian chính xác
- **Multi-tasking**: RTOS task với priority cao cho real-time monitoring

### 3. Data Management
- **Session Tracking**: Lưu trữ thông tin phiên sử dụng của 50 users
- **Energy Calculation**: Tính toán kWh và chi phí VND
- **Serial Logging**: Debug output chi tiết
- **JSON Export**: Chuẩn bị dữ liệu cho export

## 🚧 Đang Phát triển

### 1. HTTP Export Module
- **Status**: Đã tạo framework, chưa implement HTTP client
- **Files**: `include/http_export.h`, `src/http_export.cpp`
- **Current**: Simulation mode - in ra Serial thay vì gửi HTTP
- **Next**: Implement WiFiClientSecure và HTTPClient cho ESP32

### 2. Google Sheets Integration
- **Status**: Google Apps Script đã sẵn sàng
- **Files**: `google_apps_script/energy_export.gs`
- **Current**: Script hoàn chỉnh, chờ HTTP client implementation
- **Next**: Kết nối ESP32 với Google Apps Script

## 📋 Cần Làm

### 1. HTTP Client Implementation
```cpp
// Trong src/http_export.cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

bool HTTPExporter::exportToGoogleSheets(const UserSession& session) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    http.begin(client, googleScriptURL);
    http.addHeader("Content-Type", "application/json");
    
    // Create and send JSON payload
    // Handle response
}
```

### 2. Hardware Testing
- **RFID RC522**: Test card scanning và session management
- **M5 Stack AC Measure**: Verify power readings
- **I2C Communication**: Debug M5 Stack connection
- **Power Calculation**: Validate energy calculations

### 3. ThingsBoard Dashboard
- **Widgets**: Current Power, Energy Consumption, Session Cost
- **Alarms**: High power usage, long sessions
- **Rule Chains**: Automated notifications

## 🔧 Configuration

### 1. Hardware Pins
```cpp
// RFID RC522
#define RST_PIN 22
#define SS_PIN  21

// M5 Stack AC Measure
#define AC_MEASURE_I2C_ADDR 0x42
```

### 2. System Parameters
```cpp
#define ELECTRICITY_RATE 3500.0  // VND per kWh
#define POWER_SAMPLE_INTERVAL 1000  // ms
#define MAX_USERS 50
```

### 3. Google Apps Script
- **URL**: Cần thay thế trong `src/http_export.cpp`
- **Spreadsheet ID**: Cần cấu hình trong Google Apps Script
- **Permissions**: Deploy as web app với public access

## 🚀 Quick Start

### 1. Hardware Setup
1. Kết nối RFID RC522 theo sơ đồ pin
2. Kết nối M5 Stack AC Measure Unit qua I2C
3. Cấp nguồn và kiểm tra kết nối

### 2. Software Setup
1. Build và upload code lên YoloUNO
2. Monitor Serial output để debug
3. Test RFID card scanning
4. Verify ThingsBoard telemetry

### 3. Google Sheets Setup
1. Tạo Google Sheets mới
2. Deploy Google Apps Script
3. Cập nhật URL trong code
4. Test HTTP export

## 📊 Expected Output

### Serial Monitor
```
Initializing Energy Management System...
RFID RC522 initialized
M5 Stack AC Measure connected
Card detected: A1B2C3D4
Session started - Card: A1B2C3D4
Power: 150.25 W, Energy: 0.000042 kWh
...
Session ended - Card: A1B2C3D4, Energy: 0.125 kWh, Cost: 438 VND
```

### ThingsBoard Telemetry
- `current_power`: 150.25
- `accumulated_energy`: 0.125
- `device_in_use`: 1
- `session_cost`: 437.5
- `active_card_id`: "A1B2C3D4"

### Google Sheets
| Timestamp | Card ID | Start Time | End Time | Energy (kWh) | Cost (VND) |
|-----------|---------|------------|----------|--------------|------------|
| 2024-01-15 10:30:00 | A1B2C3D4 | 2024-01-15 10:00:00 | 2024-01-15 10:30:00 | 0.125 | 438 |

## 🐛 Known Issues

1. **HTTP Client**: Compilation errors với WiFiClientSecure - cần fix
2. **M5 Stack**: Chưa test thực tế với hardware
3. **Time Sync**: NTP có thể fail nếu WiFi không ổn định
4. **Memory**: Cần monitor heap usage với 50 concurrent sessions

## 📞 Support

Để debug issues:
1. Check Serial Monitor output
2. Verify hardware connections
3. Test individual components
4. Monitor ThingsBoard logs
5. Check WiFi connectivity

## 🔄 Next Release (v2.0)

- [ ] HTTP Client implementation
- [ ] Real hardware testing
- [ ] Mobile app integration
- [ ] Payment gateway
- [ ] Machine learning predictions
- [ ] Multi-device support 