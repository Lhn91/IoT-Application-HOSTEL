#  HƯỚNG DẪN SETUP HỆ THỐNG ĐIỂM DANH RFID VỚI GOOGLE SHEETS

##  Tổng quan hệ thống

Hệ thống điểm danh RFID này sẽ:
- Quét thẻ RFID để ghi nhận vào/ra
- Tự động ghi dữ liệu vào Google Sheets
- Phân biệt trạng thái IN/OUT tự động
- Lưu trữ lịch sử điểm danh đầy đủ

##  Cấu trúc 2 Google Sheets

### 1.  **Database Sheet** - Lưu thông tin thẻ và người dùng
| STT | Mã thẻ | Họ tên | Chức vụ |
|-----|--------|--------|---------|
| 1   | D46C1200 | Nguyen Van A | Sinh viên |
| 2   | EBE90F05 | Tran Thi B | Sinh viên |

### 2. 📋 **Attendance_Log Sheet** - Ghi nhận lịch sử điểm danh
| STT | Mã thẻ | Họ tên | Ngày | Giờ | Trạng thái |
|-----|--------|--------|------|-----|-----------|
| 1   | D46C1200 | Nguyen Van A | 25/12/2024 | 08:30:00 | IN |
| 2   | D46C1200 | Nguyen Van A | 25/12/2024 | 17:45:00 | OUT |

## BƯỚC 1: TẠO GOOGLE SHEETS VÀ APPS SCRIPT

### 1.1 Tạo Google Spreadsheet mới
1. Truy cập [Google Sheets](https://sheets.google.com)
2. Tạo spreadsheet mới với tên: `RFID_Attendance_System`
3. Lưu lại URL của spreadsheet (Link này của tôi: 1HdjRLNujHRQ3w0zjwyOWVY_yk0mYToVpzQB0sK4ZSuk)

### 1.2 Tạo Google Apps Script
1. Trong Google Sheets, vào **Extensions** → **Apps Script**
2. Xóa code mặc định và dán toàn bộ code từ file `google_apps_script.js`
3. Lưu project với tên: `RFID_Attendance_API`

### 1.3 Khởi tạo database
1. Trong Apps Script editor, chọn function `initializeSheets`
2. Click **Run** để tạo 2 sheets và dữ liệu mẫu
3. Grant permissions khi được yêu cầu

### 1.4 Deploy Web App
1. Click **Deploy** → **New deployment**
2. Type: **Web app**
3. Execute as: **Me**
4. Who has access: **Anyone**
5. Click **Deploy**
6. **QUAN TRỌNG**: Copy URL deployment (ví dụ: `https://script.google.com/macros/s/AKfycby.../exec`)
https://script.google.com/macros/s/AKfycbz_8TsCFTEcin1s2s862lxvxW7if1UnmjrrDKD_w-CZINDOy9x6JhpJKd7wNcRA4jW7/exec

id deploy: AKfycbz_8TsCFTEcin1s2s862lxvxW7if1UnmjrrDKD_w-CZINDOy9x6JhpJKd7wNcRA4jW7
##  BƯỚC 2: CẤU HÌNH ESP32

### 2.1 Cập nhật Google Script ID
1. Mở file `src/google_sheets_task.cpp`
2. Tìm dòng:
```cpp
const char* GOOGLE_SCRIPT_ID = "YOUR_SCRIPT_ID_HERE";
```
3. Thay thế bằng Script ID từ URL deployment (phần giữa `/macros/s/` và `/exec`)

**Ví dụ:**
Nếu URL là: `https://script.google.com/macros/s/AKfycby1234567890abcdef/exec`
Thì Script ID là: `AKfycby1234567890abcdef`

```cpp
const char* GOOGLE_SCRIPT_ID = "AKfycby1234567890abcdef";
```

### 2.2 Cập nhật database thẻ RFID
1. Trong file `src/google_sheets_task.cpp`, tìm phần `userDatabase[]`
2. Thêm/sửa thông tin thẻ của bạn:

```cpp
UserData userDatabase[] = {
  {"D46C1200", "Nguyen Van A"},
  {"EBE90F05", "Tran Thi B"},
  {"YOUR_CARD_ID", "YOUR_NAME"},  // Thêm thẻ của bạn
  {"", ""}  // Kết thúc mảng
};
```

### 2.3 Compile và upload
1. Build project với PlatformIO
2. Upload lên ESP32

##  BƯỚC 3: TEST HỆ THỐNG

### 3.1 Test Google Apps Script
1. Trong Apps Script editor, chọn function `testScript`
2. Click **Run**
3. Kiểm tra sheet `Attendance_Log` có thêm dòng test không

### 3.2 Test ESP32
1. Mở Serial Monitor (115200 baud)
2. Đưa thẻ RFID gần reader
3. Quan sát log:

```
Mã thẻ: D4 6C 12 00
Loại thẻ: MIFARE 1KB
Mã thẻ (String): D46C1200
 Processing attendance...
 Attendance: Nguyen Van A - IN - 25/12/2024 08:30:00
 Attendance sent to Google Sheets successfully!
```

4. Kiểm tra Google Sheets có dữ liệu mới không

##  BƯỚC 4: SỬ DỤNG HỆ THỐNG

### 4.1 Quy trình điểm danh
1. **Lần đầu quét thẻ**: Trạng thái **IN** (vào)
2. **Lần thứ 2 quét thẻ**: Trạng thái **OUT** (ra)
3. **Lần thứ 3 quét thẻ**: Trạng thái **IN** (vào lại)
4. Cứ thế luân phiên...

### 4.2 Thêm thẻ mới
1. **Cách 1 - Sửa code**:
   - Thêm vào `userDatabase[]` trong `google_sheets_task.cpp`
   - Compile và upload lại

2. **Cách 2 - Sửa Google Sheets**:
   - Thêm thông tin vào sheet `Database`
   - Cập nhật code để đồng bộ từ Google Sheets (nâng cao)

### 4.3 Xem báo cáo
- Truy cập Google Sheets để xem:
  - Lịch sử điểm danh chi tiết
  - Thống kê thời gian vào/ra
  - Tạo chart và dashboard

## BƯỚC 5: TÙY CHỈNH NÂNG CAO

### 5.1 Thêm múi giờ
Trong `google_sheets_task.cpp`, sửa dòng:
```cpp
configTime(7 * 3600, 0, "pool.ntp.org"); // GMT+7 cho Việt Nam
```

### 5.2 Thêm validation
- Kiểm tra thời gian hợp lệ (không cho điểm danh ngoài giờ)
- Thêm cooldown time giữa các lần quét
- Validation duplicate trong ngày

### 5.3 Thêm notification
- Gửi email khi có điểm danh
- Slack/Discord webhook
- SMS notification

### 5.4 Dashboard nâng cao
- Google Data Studio
- Real-time chart
- Export báo cáo PDF

##  TROUBLESHOOTING

### Lỗi thường gặp:

#### 1. **ESP32 không gửi được lên Google Sheets**
```
❌ Failed to send attendance to Google Sheets
```
**Giải pháp**:
- Kiểm tra WiFi connection
- Kiểm tra Google Script ID đúng chưa
- Kiểm tra Apps Script có deploy đúng không

#### 2. **Google Apps Script lỗi permission**
```
Exception: You do not have permission to call SpreadsheetApp.getActiveSpreadsheet
```
**Giải pháp**:
- Chạy lại function `initializeSheets`
- Grant tất cả permissions cần thiết

#### 3. **Thẻ RFID không nhận diện**
```
Card not registered in database!
```
**Giải pháp**:
- Kiểm tra mã thẻ in ra Serial có đúng không
- Thêm mã thẻ vào `userDatabase[]`
- Kiểm tra format mã thẻ (uppercase)

#### 4. **Thời gian không đúng**
```
Failed to obtain time
```
**Giải pháp**:
- Kiểm tra kết nối internet
- Đợi ESP32 sync NTP server
- Restart ESP32
