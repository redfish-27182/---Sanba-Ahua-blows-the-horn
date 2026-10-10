#pragma once

// 將此檔案複製為 secrets.h，再填入實際數值。secrets.h 已被 Git 忽略，
// 因此 Wi-Fi 與 MQTT 憑證不會被提交。

constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

// EMQX Serverless 必須使用完整網域名稱與 MQTTS 的 8883 埠。
constexpr char MQTT_HOST[] = "YOUR_DEPLOYMENT.ala.us-east-1.emqxsl.com";
constexpr uint16_t MQTT_PORT = 8883;
constexpr char MQTT_USERNAME[] = "YOUR_MQTT_USERNAME";
constexpr char MQTT_PASSWORD[] = "YOUR_MQTT_PASSWORD";

// 每一塊實體開發板請設定不同 ID；它會用於 MQTT 狀態主題與 Client ID。
constexpr char DEVICE_ID[] = "esp32-mqtt-version-test";
