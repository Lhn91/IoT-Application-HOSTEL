# Energy Management System - Hệ thống Quản lý Lượng Điện Tiêu Thụ

## Tổng quan

Hệ thống quản lý lượng điện tiêu thụ cho phép theo dõi và tính toán chi phí điện năng của người dùng thông qua việc quét thẻ RFID. Hệ thống tích hợp với ThingsBoard để giám sát real-time và xuất dữ liệu ra Google Sheets.

## Kiến trúc Hệ thống

### Hardware Requirements
- **YoloUNO ESP32** - Vi điều khiển chính
- **RFID Reader RC522** - Đọc thẻ RFID người dùng
- **M5 Stack AC Measure Unit** - Đo công suất AC
- **Relay Module** - Điều khiển thiết bị chung
- **Thiết bị điện cần giám sát** (máy giặt, máy sấy, v.v.)

### Software Components
- **RTOS Tasks** - Quản lý đa nhiệm
- **ThingsBoard Integration** - Giám sát real-time
- **Google Sheets Export** - Lưu trữ dữ liệu
- **NTP Time Sync** - Đồng bộ thời gian

## Cài đặt Hardware

### Kết nối RFID RC522
```
RC522    ->  YoloUNO
VCC      ->  3.3V
RST      ->  GPIO 22
GND      ->  GND
MISO     ->  GPIO 19
MOSI     ->  GPIO 23
SCK      ->  GPIO 18
SDA(SS)  ->  GPIO 21
```

### Kết nối M5 Stack AC Measure Unit
```
M5 Unit  ->  YoloUNO
VCC      ->  5V
GND      ->  GND
SDA      ->  GPIO 21 (I2C)
SCL      ->  GPIO 22 (I2C)
```

## Cài đặt Software

### 1. Cài đặt Libraries
Các thư viện đã được thêm vào `platformio.ini`:
- MFRC522 (RFID)
- ArduinoJson (JSON processing)
- NTPClient (Time synchronization)
- Time (Time management)

### 2. Cấu hình Google Apps Script

#### Bước 1: Tạo Google Sheets
1. Tạo Google Sheets mới
2. Copy Spreadsheet ID từ URL
3. Thay thế `YOUR_SPREADSHEET_ID` trong file `google_apps_script/energy_export.gs`

#### Bước 2: Deploy Apps Script
1. Mở [Google Apps Script](https://script.google.com)
2. Tạo project mới
3. Copy nội dung từ `google_apps_script/energy_export.gs`
4. Deploy as Web App:
   - Execute as: Me
   - Who has access: Anyone
5. Copy Web App URL
6. Thay thế URL trong `src/energy_management.cpp` tại dòng:
   ```cpp
   http.begin("https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec");
   ```

### 3. Cấu hình ThingsBoard

#### Telemetry Keys mới:
- `current_power` - Công suất hiện tại (W)
- `accumulated_energy` - Điện năng tích lũy (kWh)
- `device_in_use` - Trạng thái sử dụng thiết bị
- `session_start` - Bắt đầu phiên sử dụng
- `session_end` - Kết thúc phiên sử dụng
- `session_energy` - Điện năng phiên (kWh)
- `session_cost` - Chi phí phiên (VND)
- `session_duration` - Thời gian phiên (giây)
- `active_card_id` - ID thẻ đang sử dụng

## Cách sử dụng

### 1. Quy trình sử dụng cơ bản
1. **Bắt đầu sử dụng**: Quét thẻ RFID lần đầu
   - Hệ thống bắt đầu đo công suất
   - Ghi nhận thời gian bắt đầu
   - Gửi telemetry lên ThingsBoard

2. **Trong quá trình sử dụng**:
   - M5 Stack đo công suất liên tục
   - Tính toán điện năng tích lũy
   - Gửi dữ liệu real-time lên ThingsBoard

3. **Kết thúc sử dụng**: Quét thẻ RFID lần thứ hai
   - Tính toán tổng điện năng tiêu thụ
   - Tính chi phí dựa trên giá điện
   - Xuất dữ liệu ra Google Sheets
   - Gửi thông tin phiên lên ThingsBoard

### 2. Giám sát trên ThingsBoard

#### Dashboard Widgets cần thêm:
- **Current Power Chart** - Biểu đồ công suất real-time
- **Energy Consumption** - Tổng điện năng tiêu thụ
- **Session Cost** - Chi phí phiên hiện tại
- **Device Status** - Trạng thái thiết bị
- **Active User** - Thẻ đang sử dụng

#### Rule Chains có thể thêm:
- **High Power Alert** - Cảnh báo công suất cao
- **Long Session Alert** - Cảnh báo phiên sử dụng quá lâu
- **Cost Threshold** - Cảnh báo chi phí vượt ngưỡng

### 3. Báo cáo Excel

Google Sheets sẽ tự động tạo bảng với các cột:
- Timestamp - Thời gian ghi nhận
- Card ID - Mã thẻ người dùng
- Start Time - Thời gian bắt đầu
- End Time - Thời gian kết thúc
- Duration - Thời gian sử dụng (giây)
- Total Energy - Tổng điện năng (kWh)
- Average Power - Công suất trung bình (W)
- Cost - Chi phí (VND)
- Device Name - Tên thiết bị

## Cấu hình

### 1. Thay đổi giá điện
Trong file `include/energy_management.h`:
```cpp
#define ELECTRICITY_RATE 3500.0  // VND per kWh
```

### 2. Cấu hình I2C Address cho M5 Stack
```cpp
#define AC_MEASURE_I2C_ADDR 0x42
```

### 3. Cấu hình RFID Pins
```cpp
#define RST_PIN 22
#define SS_PIN  21
```

### 4. Cấu hình thời gian sampling
```cpp
#define POWER_SAMPLE_INTERVAL 1000  // ms
```

## Troubleshooting

### 1. RFID không đọc được thẻ
- Kiểm tra kết nối SPI
- Đảm bảo nguồn 3.3V ổn định
- Kiểm tra khoảng cách thẻ với reader

### 2. M5 Stack không đọc được công suất
- Kiểm tra kết nối I2C
- Verify I2C address (0x42)
- Đảm bảo M5 Stack được cấp nguồn đúng

### 3. Không xuất được Excel
- Kiểm tra kết nối WiFi
- Verify Google Apps Script URL
- Kiểm tra quyền truy cập Google Sheets

### 4. ThingsBoard không nhận dữ liệu
- Kiểm tra MQTT connection
- Verify device token
- Kiểm tra mutex locks

## API Reference

### EnergyManager Class Methods

```cpp
// Khởi tạo hệ thống
void begin();

// Vòng lặp chính
void loop();

// Kiểm tra thẻ RFID
bool isCardPresent();

// Xử lý quét thẻ
void handleCardScan();

// Bắt đầu phiên sử dụng
void startSession(const String& cardId);

// Kết thúc phiên sử dụng
void endSession(const String& cardId);

// Lấy công suất hiện tại
float getCurrentPower();

// Kiểm tra trạng thái thiết bị
bool isDeviceInUse();

// Gửi telemetry lên ThingsBoard
void sendEnergyTelemetry();
```

## Mở rộng

### 1. Thêm nhiều thiết bị
- Sử dụng relay matrix
- Thêm device ID vào session
- Cấu hình giá điện khác nhau cho từng thiết bị

### 2. Tích hợp thanh toán
- Thêm API payment gateway
- Tự động trừ tiền từ tài khoản
- SMS/Email notification

### 3. Machine Learning
- Dự đoán mức tiêu thụ
- Phát hiện bất thường
- Tối ưu hóa sử dụng điện

### 4. Mobile App
- React Native/Flutter app
- Real-time monitoring
- Push notifications
- QR code scanning

## Bảo mật

### 1. RFID Security
- Sử dụng encrypted RFID cards
- Implement card authentication
- Regular key rotation

### 2. Data Security
- HTTPS cho tất cả API calls
- Encrypt sensitive data
- Access control cho Google Sheets

### 3. Network Security
- WPA3 WiFi encryption
- VPN cho remote access
- Firewall rules

## Performance

### Typical Performance Metrics:
- **RFID Response Time**: < 500ms
- **Power Measurement Frequency**: 1Hz
- **ThingsBoard Update**: 5s interval
- **Memory Usage**: ~60% of ESP32
- **Power Consumption**: ~150mA @ 5V

## Support

Để được hỗ trợ:
1. Kiểm tra Serial Monitor output
2. Verify hardware connections
3. Test individual components
4. Check network connectivity
5. Review ThingsBoard logs

## License

MIT License - Xem file LICENSE để biết thêm chi tiết. 