#pragma once

#include <Arduino.h>

// Queue 傳遞固定大小的資料。
constexpr size_t APP_TOPIC_LENGTH = 80; // MQTT 主題長度
constexpr size_t APP_PAYLOAD_LENGTH = 128; // MQTT 訊息內容長度
constexpr size_t APP_VERSION_LENGTH = 24;  // 版本號長度

// 後端推送且需要更新遊戲畫面的天氣狀態。
struct WeatherState {
    int16_t temperatureTenthsC; // 攝氏溫度乘以 10，例如 265 代表 26.5°C。
    uint16_t conditionCode;     // WeatherAPI 天氣代碼。
    uint8_t rainProbability;    // 降雨機率，範圍 0 到 100。
    int16_t utcOffsetMinutes;   // 後端依定位地點計算的 UTC 偏移分鐘數。
    uint32_t observedAt;        // 後端提供的觀測 Unix 時間。
};

// 由 MqttTask 轉交給 UiTask 的已驗證事件類型。
enum class UiEventType : uint8_t {
    FIRMWARE_UPDATE,
    WEATHER_UPDATED,
};

// 韌體更新事件所需的最小資料。
struct FirmwareUpdate {
    char version[APP_VERSION_LENGTH];
};

// 單一 UI Queue 的訊息容器；同時間只使用其中一種資料。
struct UiEvent {
    UiEventType type;
    union {
        FirmwareUpdate firmware;
        WeatherState weather;
    };
};

// 其他 Task 要求 MQTT Task 發送的訊息。
struct MqttPublishRequest {
    char topic[APP_TOPIC_LENGTH];     // 主題
    char payload[APP_PAYLOAD_LENGTH]; // 內容
    bool retained;                    // 是否保留訊息
};
