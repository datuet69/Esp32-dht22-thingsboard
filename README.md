# ESP32 DHT22 ThingsBoard Monitor

## 1. Giới thiệu

Dự án sử dụng ESP32 và cảm biến DHT22 để đo nhiệt độ,
độ ẩm và gửi dữ liệu lên ThingsBoard Cloud thông qua
giao thức MQTT.

## 2. Chức năng

- Đọc nhiệt độ từ DHT22.
- Đọc độ ẩm từ DHT22.
- Hiệu chỉnh sai số nhiệt độ.
- Kết nối Wi-Fi.
- Gửi dữ liệu lên ThingsBoard bằng MQTT.
- Gửi cường độ tín hiệu Wi-Fi RSSI.
- Tạo trạng thái cảnh báo nhiệt độ và độ ẩm.

## 3. Phần cứng

- ESP32 Development Board.
- DHT22 temperature and humidity sensor.
- Breadboard.
- Jumper wires.
- USB cable.

## 4. Kết nối phần cứng

| DHT22 | ESP32 |
|---|---|
| VCC | 3V3 |
| DATA | GPIO4 |
| GND | GND |

## 5. Cấu trúc thư mục

```text
esp32-dht22-thingsboard/
├── include/
│   ├── secrets.example.h
│   └── secrets.h
├── src/
│   └── main.cpp
├── .gitignore
├── platformio.ini
└── README.md
```

## 6. Cấu hình thông tin bảo mật

Copy file:

```text
include/secrets.example.h
```

thành:

```text
include/secrets.h
```

Sau đó điền thông tin Wi-Fi và ThingsBoard token:

```cpp
#pragma once

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define THINGSBOARD_TOKEN "YOUR_DEVICE_ACCESS_TOKEN"
```

Không được đưa file `secrets.h` lên GitHub.

## 7. Build chương trình

```bash
pio run
```

## 8. Nạp chương trình

```bash
pio run --target upload
```

## 9. Mở Serial Monitor

```bash
pio device monitor
```

Baud rate:

```text
115200
```

## 10. ThingsBoard telemetry

Thiết bị gửi các dữ liệu:

- `temperature`
- `humidity`
- `temperatureAlarm`
- `humidityAlarm`
- `rssi`

## 11. ThingsBoard server

```text
eu.thingsboard.cloud
```

MQTT port:

```text
1883
```

## 12. Tình trạng hiện tại

- [x] Đọc DHT22.
- [x] Kết nối Wi-Fi.
- [x] Gửi MQTT lên ThingsBoard.
- [x] Đưa mã nguồn vào PlatformIO.
- [ ] Thêm Deep Sleep.
- [ ] Đo dòng tiêu thụ.
- [ ] Thêm đo điện áp pin.
- [ ] Thêm GitHub Actions.
- [ ] Thử nghiệm nhiều thiết bị.

## 13. Bảo mật

Các thông tin sau không được lưu trên GitHub:

- Wi-Fi password.
- ThingsBoard access token.
- API keys.
- Private credentials.
## Deep Sleep

The device operates using the following cycle:

1. Wake up.
2. Read temperature and humidity.
3. Connect to Wi-Fi.
4. Connect to ThingsBoard.
5. Publish telemetry.
6. Disconnect Wi-Fi.
7. Enter Deep Sleep.
8. Wake up using the RTC timer.

The test sleep interval is 30 seconds.

Telemetry keys:

- temperature
- humidity
- temperatureAlarm
- humidityAlarm
- rssi
- wakeCount