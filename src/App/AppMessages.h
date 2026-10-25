#pragma once

#include <Arduino.h>

// Queue 傳遞固定大小的資料。
constexpr size_t APP_TOPIC_LENGTH = 80; // MQTT 主題長度
constexpr size_t APP_PAYLOAD_LENGTH = 128; // MQTT 訊息內容長度
constexpr size_t APP_VERSION_LENGTH = 24;  // 版本號長度

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
