#pragma once

// 將此檔複製成 Secrets.h 後，填入 EMQX Cloud 的連線資料。
// Secrets.h 已列入 .gitignore，帳密不會被 Git 提交。
constexpr char MQTT_HOST[] = "your-deployment.emqxsl.com";
constexpr uint16_t MQTT_PORT = 8883;
constexpr char MQTT_USERNAME[] = "your-username";
constexpr char MQTT_PASSWORD[] = "your-password";

// 同一台裝置應使用固定且唯一的 ID，供 MQTT 連線與狀態主題使用。
constexpr char MQTT_DEVICE_ID[] = "ahou-esp32-01";
