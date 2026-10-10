#pragma once

#include <Arduino.h>

// Queue 傳遞固定大小的資料。
constexpr size_t APP_TOPIC_LENGTH = 80;
constexpr size_t APP_PAYLOAD_LENGTH = 128;
constexpr size_t APP_VERSION_LENGTH = 24;
constexpr size_t APP_MESSAGE_LENGTH = 128;

// MQTT Task 收到的原始訊息。
struct MqttInboundEvent {
    char topic[APP_TOPIC_LENGTH];
    char payload[APP_PAYLOAD_LENGTH];
};

// 其他 Task 要求 MQTT Task 發送的訊息。
struct MqttPublishRequest {
    char topic[APP_TOPIC_LENGTH];     // 主題
    char payload[APP_PAYLOAD_LENGTH]; // 內容
    bool retained;                    // 是否保留訊息
};

// UiTask 顯示訊息，並等待使用者回應。
enum class UiCommandType : uint8_t {
    SHOW_UPDATE_PROMPT, // 顯示更新提示
    SHOW_NOTICE,        // 顯示一般訊息 
    PREPARE_FOR_OTA,    // UiTask 停止一般畫面繪製，讓給 OTA 使用
    RESUME_NORMAL_UI,   // OTA 結束，UiTask 恢復一般畫面繪製
    SET_WIFI_ONLINE,    // 顯示 WiFi 已連線
    SET_WIFI_OFFLINE,   // 顯示 WiFi 已斷線
    SET_MQTT_ONLINE,    // 顯示 MQTT 已連線
    SET_MQTT_OFFLINE    // 顯示 MQTT 已斷線
};

// 只有 UiTask 可以消費此訊息並使用 TFT；其他 Task 不直接畫畫面。
struct UiCommand {
    UiCommandType type;
    char version[APP_VERSION_LENGTH];
    char message[APP_MESSAGE_LENGTH];
};

// UiTask 回覆 UpdateManager：使用者選擇，以及 TFT 已讓給 OTA 使用的確認。
enum class UiResponseType : uint8_t {
    UPDATE_ACCEPTED, // 使用者同意更新
    UPDATE_DECLINED, // 使用者拒絕更新
    OTA_SCREEN_READY // UiTask 已經讓給 OTA 使用，並顯示版本號
};

// UiTask 回覆 UpdateManager：使用者選擇，以及 TFT 已讓給 OTA 使用的確認。
struct UiResponse {
    UiResponseType type;
    char version[APP_VERSION_LENGTH];
};
