# 🔧 Setup Google Sheets Integration

## ✅ Hệ thống đã sẵn sàng!

HTTP Export đã được **triệt để sửa chữa** và **build thành công**. Bây giờ chỉ cần setup Google Apps Script để hoàn thiện.

## 📋 Bước 1: Tạo Google Sheets

1. Mở [Google Sheets](https://sheets.google.com)
2. Tạo spreadsheet mới
3. Đặt tên: "Energy Management System"
4. Copy **Spreadsheet ID** từ URL:
   ```
   https://docs.google.com/spreadsheets/d/1ABC123DEF456GHI789JKL/edit
                                    ↑
                              Đây là Spreadsheet ID
   ```

## 📋 Bước 2: Deploy Google Apps Script

1. Mở [Google Apps Script](https://script.google.com)
2. Tạo project mới: **"Energy Export Handler"**
3. Xóa code mặc định và paste code từ `google_apps_script/energy_export.gs`
4. **Sửa dòng 4**:
   ```javascript
   const SPREADSHEET_ID = 'YOUR_SPREADSHEET_ID_HERE';
   //                      ↑
   //              Thay bằng ID thực từ bước 1
   ```

5. **Deploy as Web App**:
   - Click **Deploy** → **New deployment**
   - Type: **Web app**
   - Execute as: **Me**
   - Who has access: **Anyone**
   - Click **Deploy**

6. **Copy Web App URL**:
   ```
   https://script.google.com/macros/s/AKfycbw.../exec
   ```

## 📋 Bước 3: Cập nhật ESP32 Code

Sửa file `src/http_export.cpp` dòng 104:

```cpp
// Thay URL này:
httpExporter.begin("https://script.google.com/macros/s/AKfycbwPZwZRPMIcG-6a8UjRLOklPzxo0YBDZHG_nypcgoOxaQaEpog8eT2IY9K7XEMd4cc1/exec");

// Bằng URL thực từ Google Apps Script:
httpExporter.begin("YOUR_ACTUAL_GOOGLE_APPS_SCRIPT_URL");
```

## 📋 Bước 4: Build và Upload

```bash
C:\Users\84859\.platformio\penv\Scripts\platformio.exe run --target upload
```

## 🧪 Test Hệ thống

### 1. Monitor Serial Output
```bash
C:\Users\84859\.platformio\penv\Scripts\platformio.exe device monitor
```

### 2. Test RFID Workflow
1. **Quét RFID lần 1** → Bắt đầu session
2. **Đợi vài giây** → Hệ thống đo power
3. **Quét RFID lần 2** → Kết thúc session → Export to Excel

### 3. Expected Serial Output
```
=== REAL HTTP EXPORT TO GOOGLE SHEETS ===
URL: https://script.google.com/macros/s/AKfyc.../exec
JSON Payload: {"cardId":"A1B2C3D4","startTime":1705123456...}
Sending HTTPS POST request...
Raw Response:
HTTP/1.1 200 OK
{"status":"success","message":"Data saved successfully","row":15}
✅ Excel export successful!
==========================================
```

## 📊 Kết quả trong Google Sheets

| Timestamp | Card ID | Start Time | End Time | Duration | Energy (kWh) | Power (W) | Cost (VND) | Device |
|-----------|---------|------------|----------|----------|--------------|-----------|------------|---------|
| 2024-01-15 10:30:00 | A1B2C3D4 | 2024-01-15 10:00:00 | 2024-01-15 10:30:00 | 1800 | 0.1250 | 150.25 | 438 | Shared Device |

## 🔧 Troubleshooting

### ❌ "Connection failed"
- Check WiFi connection
- Verify internet access
- Ensure not in AP mode

### ❌ "HTTP 404"
- Verify Google Apps Script URL
- Check deployment settings
- Ensure "Anyone" access permission

### ❌ "No response received"
- Check network stability
- Verify Google Apps Script is deployed
- Test with browser: paste URL + add `?test=1`

### ❌ "Invalid JSON format"
- Check ESP32 JSON payload in Serial Monitor
- Verify all required fields are present

## 🎯 Tính năng hoàn chỉnh

### ✅ Đã implement:
- ✅ **Real HTTPS POST** (không còn simulation)
- ✅ **WiFiClientSecure** với SSL support
- ✅ **Error handling** đầy đủ
- ✅ **Timeout protection** (10 giây)
- ✅ **JSON payload** hoàn chỉnh
- ✅ **Google Apps Script** với validation
- ✅ **Auto-formatting** trong Sheets
- ✅ **Build thành công** không lỗi

### 🔄 Workflow hoàn chỉnh:
```
RFID Scan → Session Start → Power Measurement → RFID Scan → 
Session End → Calculate Cost → HTTP POST → Google Sheets → 
ThingsBoard Telemetry → Serial Log
```

## 🚀 Ready to Use!

Hệ thống **100% sẵn sàng** để xuất Excel thực sự. Chỉ cần:
1. Setup Google Apps Script (5 phút)
2. Thay URL trong ESP32 code (1 dòng)
3. Upload và test

**Không còn simulation mode** - đây là **real HTTP export** hoàn chỉnh! 