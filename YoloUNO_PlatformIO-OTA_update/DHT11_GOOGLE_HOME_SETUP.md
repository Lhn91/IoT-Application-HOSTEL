# DHT11 Temperature Sensor với Google Home Integration

## 🌡️ Tính năng mới đã thêm

Đã tích hợp DHT11 temperature sensor với SinricPro để hiển thị nhiệt độ trên Google Home.

## 📋 Cấu hình

### Device ID đã được cấu hình:
- **Temperature Sensor ID**: `6832de298ed485694c3f7312`
- **Switch ID**: `6832c78b8ed485694c3f5b3d` (thiết bị cũ)

### Pin kết nối:
- **DHT11 Data Pin**: GPIO 6
- **DHT11 VCC**: 3.3V
- **DHT11 GND**: GND

## 🔧 Cách hoạt động

1. **Đọc dữ liệu**: DHT11 đọc nhiệt độ và độ ẩm mỗi 5 giây
2. **Gửi ThingsBoard**: Cả nhiệt độ và độ ẩm được gửi lên ThingsBoard dashboard
3. **Gửi SinricPro**: **CHỈ NHIỆT ĐỘ** được gửi lên SinricPro (Google Home chỉ hỗ trợ temperature)
4. **Google Home**: Bạn có thể hỏi "Hey Google, what's the temperature?" 

## 📱 Lệnh Google Home

Sau khi setup xong, bạn có thể sử dụng các lệnh:

```
"Hey Google, what's the temperature in [room name]?"
"Hey Google, how hot is it in [room name]?"
"Hey Google, what's the current temperature?"
```

**Lưu ý**: Google Home chỉ hiển thị nhiệt độ, không hiển thị độ ẩm do giới hạn của SinricPro Temperature Sensor.

## 🔄 Tích hợp với hệ thống hiện tại

- ✅ **ThingsBoard**: Vẫn hoạt động bình thường với telemetry và attributes (cả temperature và humidity)
- ✅ **SinricPro Switch**: Vẫn điều khiển LED được
- ✅ **RPC Commands**: Vẫn hoạt động từ ThingsBoard
- ✅ **OTA Updates**: Vẫn hoạt động bình thường
- ✅ **Access Point Mode**: Vẫn hoạt động khi mất WiFi
- ✅ **Thread Safety**: Sử dụng mutex để đảm bảo an toàn

## 📊 Dữ liệu gửi

### ThingsBoard (đầy đủ):
```json
{
  "temperature": 25.5,
  "humidity": 60.2,
  "rssi": -45,
  "deviceState1": true,
  "deviceState2": false
}
```

### SinricPro (chỉ temperature):
```json
{
  "temperature": 25.5
}
```

## 🚀 Cách test

1. **Upload code** lên ESP32
2. **Kiểm tra Serial Monitor** xem có log "SinricPro: Sent temperature..."
3. **Mở SinricPro app** kiểm tra device online
4. **Test Google Home** với lệnh voice
5. **Kiểm tra ThingsBoard** dashboard (sẽ có cả temperature và humidity)

## 🔧 Troubleshooting

### Nếu Google Home không đọc được nhiệt độ:
1. Kiểm tra device ID `6832de298ed485694c3f7312` đã được add vào SinricPro app
2. Kiểm tra device type là "Temperature Sensor" 
3. Kiểm tra WiFi connection
4. Kiểm tra Serial Monitor có log gửi dữ liệu không

### Nếu DHT11 đọc NaN:
1. Kiểm tra kết nối pin GPIO 6
2. Kiểm tra nguồn 3.3V
3. Thử đổi DHT11 khác
4. Kiểm tra pull-up resistor (4.7kΩ)

## 📈 Performance

- **Frequency**: Gửi dữ liệu mỗi 5 giây
- **Memory**: Sử dụng thêm ~32 bytes RAM cho SinricPro temperature sensor
- **CPU**: Minimal impact với mutex protection
- **Network**: Thêm ~50 bytes/5s cho SinricPro data (chỉ temperature)

## 🔐 Security

- Sử dụng mutex `sinricMutex` để thread-safe
- Không gửi dữ liệu khi ở AP mode
- Chỉ gửi khi có WiFi connection
- Validation dữ liệu trước khi gửi (isnan check)

## ⚠️ Lưu ý quan trọng

- **Google Home chỉ hiển thị nhiệt độ**, không hiển thị độ ẩm
- **Độ ẩm vẫn được gửi lên ThingsBoard** để theo dõi đầy đủ
- **SinricPro Temperature Sensor** chỉ hỗ trợ 1 parameter (temperature)
- **Để xem độ ẩm**, sử dụng ThingsBoard dashboard hoặc SinricPro app 