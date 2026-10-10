#pragma once

#include <Arduino.h>

// Queue 傳遞固定大小的資料，避免不同 Task 共用 String 造成記憶體與所有權問題。
constexpr size_t APP_TOPIC_LENGTH = 80;
constexpr size_t APP_PAYLOAD_LENGTH = 128;
constexpr size_t APP_VERSION_LENGTH = 24;
constexpr size_t APP_MESSAGE_LENGTH = 128;

// MQTT Task 收到的原始訊息。它只負責收取與轉送，不在 callback 內做耗時工作。
struct MqttInboundEvent {
    char topic[APP_TOPIC_LENGTH];
    char payload[APP_PAYLOAD_LENGTH];
};

// 其他 Task 要求 MQTT Task 發送的訊息。
// 現階段主要用於未來擴充；MQTT Task 自己的 online/offline 狀態不需要繞經此 Queue。
struct MqttPublishRequest {
    char topic[APP_TOPIC_LENGTH];
    char payload[APP_PAYLOAD_LENGTH];
    bool retained;
};

// UiTask 顯示訊息，並等待使用者回應。
enum class UiCommandType : uint8_t {
    SHOW_UPDATE_PROMPT,
    SHOW_NOTICE,
    PREPARE_FOR_OTA,
    RESUME_NORMAL_UI,
    SET_WIFI_ONLINE,
    SET_WIFI_OFFLINE,
    SET_MQTT_ONLINE,
    SET_MQTT_OFFLINE
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
