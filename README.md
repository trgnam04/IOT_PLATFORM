# Dự án Kiểm thử Xác thực X.509 trên ThingsBoard

Dự án này là một môi trường thực nghiệm hoàn chỉnh để triển khai và kiểm thử các phương thức xác thực bảo mật cho thiết bị IoT trên nền tảng ThingsBoard Community Edition. Trọng tâm của dự án là xác thực bằng chứng chỉ **X.509 đơn lẻ** và **chuỗi chứng chỉ X.509 (Chain of Trust)** qua các giao thức MQTTS và CoAPS.

## Mục lục
- [Giới thiệu](#giới-thiệu)
- [Yêu cầu hệ thống](#yêu-cầu-hệ-thống)
- [Cấu trúc thư mục](#cấu-trúc-thư-mục)
- [Hướng dẫn Cài đặt và Triển khai](#hướng-dẫn-cài-đặt-và-triển-khai)
  - [Bước 1: Chuẩn bị Chứng chỉ](#bước-1-chuẩn-bị-chứng-chỉ)
  - [Bước 2: Triển khai ThingsBoard Edge](#bước-2-triển-khai-thingsboard-edge)
  - [Bước 3: Cài đặt Công cụ Kiểm thử](#bước-3-cài-đặt-công-cụ-kiểm-thử)
  - [Bước 4: Kiểm thử kết nối từ Terminal](#bước-4-kiểm-thử-kết-nối-từ-terminal)
  - [Bước 5: Nạp chương trình cho ESP32](#bước-5-nạp-chương-trình-cho-esp32)
- [Dọn dẹp](#dọn-dẹp)

---

## Giới thiệu
Dự án cung cấp một kịch bản "end-to-end" bao gồm:
- **Hạ tầng khóa công khai (PKI):** Các script và chứng chỉ đã được tạo sẵn để mô phỏng một Root CA, Intermediate CA và các chứng chỉ cho thiết bị.
- **Nền tảng IoT:** File Docker Compose để triển khai ThingsBoard CE đã được cấu hình sẵn để chấp nhận kết nối MQTTS và CoAPS với xác thực hai chiều (Two-Way SSL/mTLS).
- **Thiết bị mô phỏng:** Các câu lệnh để kiểm thử kết nối bằng công cụ Mosquitto (MQTT) và coap-client (CoAP).
- **Thiết bị thực tế:** Mã nguồn cho module ESP32 (sử dụng PlatformIO) để kết nối đến ThingsBoard qua MQTTS.

---

## Yêu cầu hệ thống
Để chạy dự án này, bạn cần cài đặt các công cụ sau trên máy của mình:
- **Docker** và **Docker Compose**: Để triển khai ThingsBoard.
- **OpenSSL**: Để tạo và quản lý chứng chỉ (thường đã có sẵn trên Linux/macOS).
- **Git**: Để clone dự án.
- **Công cụ kiểm thử dòng lệnh**:
  - `mosquitto-clients`: Để sử dụng `mosquitto_pub`.
  - `libcoap2-openssl` (trên Debian/Ubuntu): Để sử dụng `coap-client-openssl`.
- **Môi trường phát triển cho ESP32**:
  - **Visual Studio Code**
  - **Tiện ích mở rộng PlatformIO IDE**

---

## Cấu trúc thư mục
```
.
├── iot_platform_cert/        # Chứa tất cả chứng chỉ và khóa của PKI
├── iot_platform_deploy/      # Chứa file Docker Compose để triển khai ThingsBoard
├── iot_platform_device/      # Chứa mã nguồn firmware cho module ESP32
└── iot_platform_test_tools/  # Nơi lưu trữ các công cụ hỗ trợ (nếu cần tải về)
```
- `iot_platform_cert`: Nơi quản lý toàn bộ hạ tầng khóa. Các chứng chỉ đã được tạo sẵn để bạn có thể sử dụng ngay.
- `iot_platform_deploy`: Chứa file `docker-compose.yml` để khởi chạy ThingsBoard và cơ sở dữ liệu.
- `iot_platform_device`: Chứa các project PlatformIO cho ESP32, mỗi project tương ứng với một kịch bản xác thực.
- `iot_platform_test_tools`: Thư mục trống để bạn có thể chứa các file thực thi của các công cụ kiểm thử nếu bạn không cài đặt chúng vào hệ thống.

---

## Hướng dẫn Cài đặt và Triển khai

### Bước 1: Chuẩn bị Chứng chỉ

Tất cả các chứng chỉ cần thiết đã được tạo sẵn trong thư mục `iot_platform_cert`. Bạn có thể sử dụng chúng ngay lập tức. Tuy nhiên, nếu bạn muốn tự tạo lại từ đầu, hãy làm theo các bước tùy chọn dưới đây.

#### Tùy chọn: Tự tạo lại Chứng chỉ từ đầu

##### a. Tạo Chứng chỉ cho Server
Các chứng chỉ này được dùng bởi ThingsBoard để kích hoạt MQTTS và CoAPS.

1.  Di chuyển vào thư mục chứa chứng chỉ của server:
    ```bash
    cd iot_platform_cert/server_cert
    ```

2.  Tạo khóa và chứng chỉ cho CA (Certificate Authority) của riêng bạn. CA này sẽ ký cho chứng chỉ của server.
    ```bash
    # Tạo khóa cho CA
    openssl genpkey -algorithm RSA -out ca_key.pem -aes256 -pass pass:secret
    
    # Tạo chứng chỉ tự ký cho CA (có hiệu lực 1 năm)
    openssl req -x509 -new -nodes -key ca_key.pem -sha256 -days 365 -out ca.pem -passin pass:secret
    ```

3.  Tạo khóa và chứng chỉ cho Server, sau đó dùng CA vừa tạo để ký.
    ```bash
    # Tạo khóa riêng cho server
    openssl genpkey -algorithm RSA -out server_key.pem -aes256 -pass pass:secret
    
    # Tạo Certificate Signing Request (CSR) cho server
    openssl req -new -key server_key.pem -out server.csr -passin pass:secret
    ```
    **QUAN TRỌNG:** Khi được hỏi `Common Name (CN)`, bạn **PHẢI** nhập địa chỉ IP của máy tính đang chạy Docker hoặc `localhost` nếu bạn kết nối từ cùng một máy. Ví dụ: `192.168.1.10`. Điều này là bắt buộc để client có thể xác thực hostname của server.

    ```bash
    # Dùng CA để ký CSR và tạo ra chứng chỉ cho server
    openssl x509 -req -in server.csr -CA ca.pem -CAkey ca_key.pem -CAcreateserial -out server.pem -days 365 -sha256 -passin pass:secret
    ```
4.  Di chuyển trở lại thư mục gốc của dự án:
    ```bash
    cd ../..
    ```

##### b. Tạo Chứng chỉ cho Thiết bị
1.  **Tạo chứng chỉ đơn lẻ (test-device2):**
    ```bash
    # Di chuyển vào thư mục của thiết bị
    cd iot_platform_cert/test_device2_cert
    
    # Tạo khóa và chứng chỉ tự ký cho thiết bị
    openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -sha256 -days 365 -nodes
    
    # Di chuyển về thư mục gốc
    cd ../..
    ```

2.  **Tạo chuỗi chứng chỉ (test-device1):** Quy trình này phức tạp hơn, bạn có thể tham khảo tài liệu của ThingsBoard hoặc sử dụng các file đã được tạo sẵn.

### Bước 2: Triển khai ThingsBoard Edge

1.  Mở terminal và di chuyển đến thư mục triển khai:
    ```bash
    cd iot_platform_deploy/thingsboard_ce
    ```

2.  Khởi chạy các container bằng Docker Compose:
    ```bash
    docker-compose up -d
    ```
    Lệnh này sẽ khởi tạo container ThingsBoard và PostgreSQL. File `docker-compose.yml` đã được cấu hình để:
    - Mở các cổng `8080` (HTTP), `1883` (MQTT), `8883` (MQTTS), `5683-5688` (CoAP/CoAPS).
    - Kích hoạt MQTTS và CoAPS.
    - Mount thư mục `../../iot_platform_cert/server_cert/` vào `/data/certs` trong container để sử dụng chứng chỉ server.

3.  Truy cập giao diện web của ThingsBoard tại `http://localhost:8080` với tài khoản mặc định:
    - **Username:** `sysadmin@thingsboard.org`
    - **Password:** `sysadmin`

### Bước 3: Cài đặt Công cụ Kiểm thử

Trên hệ thống Debian/Ubuntu, chạy lệnh sau:
```bash
sudo apt-get update && sudo apt-get install -y mosquitto-clients libcoap2-openssl
```
Đối với các hệ điều hành khác, vui lòng tham khảo tài liệu để cài đặt các công cụ tương ứng.

### Bước 4: Kiểm thử kết nối từ Terminal

**Quan trọng:** Thay thế `YOUR_SERVER_IP_OR_DOMAIN` bằng địa chỉ IP hoặc domain của máy đang chạy Docker (chính là giá trị bạn đã nhập cho `Common Name` ở Bước 1). Nếu chạy trên cùng một máy, bạn có thể dùng `localhost`.

#### a. MQTTs - Chứng chỉ Đơn lẻ (test-device2)
```bash
mosquitto_pub -d \
  -h YOUR_SERVER_IP_OR_DOMAIN -p 8883 \
  --cafile iot_platform_cert/server_cert/ca.pem \
  --cert iot_platform_cert/test_device2_cert/cert.pem \
  --key iot_platform_cert/test_device2_cert/key.pem \
  -t "v1/devices/me/telemetry" \
  -m '{"temperature":25}'
```

#### b. MQTTs - Chuỗi Chứng chỉ (test-device1)
```bash
mosquitto_pub -d \
  -h YOUR_SERVER_IP_OR_DOMAIN -p 8883 \
  --cafile iot_platform_cert/test_device1_cert/rootCert.pem \
  --cert iot_platform_cert/test_device1_cert/chain.pem \
  --key iot_platform_cert/test_device1_cert/deviceKey.pem \
  -t "v1/devices/me/telemetry" \
  -m '{"temperature":26, "type":"chain"}'
```

#### c. CoAPS - Chứng chỉ Đơn lẻ (test-device3)
```bash
coap-client-openssl -m POST -v 9 \
  -C iot_platform_cert/server_cert/ca.pem \
  -c iot_platform_cert/test_device3_cert/cert.pem \
  -j iot_platform_cert/test_device3_cert/key.pem \
  -t "application/json" \
  -e '{"temperature":43}' \
  "coaps://YOUR_SERVER_IP_OR_DOMAIN:5688/api/v1/X509/telemetry"
```

### Bước 5: Nạp chương trình cho ESP32

1.  Mở Visual Studio Code với tiện ích PlatformIO đã được cài đặt.
2.  Mở một trong các project trong thư mục `iot_platform_device`:
    - `esp32_test_x509-connection_basic`: Cho kịch bản chứng chỉ đơn lẻ.
    - `esp32_test_x509-connection`: Cho kịch bản chuỗi chứng chỉ.
3.  Mở file `src/main.cpp` và cập nhật thông tin WiFi của bạn:
    ```cpp
    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    ```
4.  Các file chứng chỉ và khóa đã được nhúng sẵn trong file `src/certs.h`.
5.  Kết nối module ESP32 vào máy tính.
6.  Sử dụng chức năng **"Upload"** của PlatformIO để biên dịch và nạp chương trình.
7.  Mở **Serial Monitor** để theo dõi log kết nối từ thiết bị.

---

## Dọn dẹp
Để dừng và xóa toàn bộ môi trường ThingsBoard, di chuyển đến thư mục `iot_platform_deploy/thingsboard_ce` và chạy lệnh:
```bash
docker-compose down -v
```
Lệnh này sẽ xóa các container, network và volume đã được tạo.

