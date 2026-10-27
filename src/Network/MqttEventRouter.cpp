#include "Network/MqttEventRouter.h"

#include <ArduinoJson.h>
#include <cmath>
#include <cstring>

#include "App/AppMessages.h"
#include "App/AppQueues.h"
#include "Network/MqttTopics.h"

namespace {
constexpr size_t WEATHER_JSON_CAPACITY = 128;
constexpr float MIN_TEMPERATURE_C = -60.0f;
constexpr float MAX_TEMPERATURE_C = 70.0f;
constexpr int MIN_UTC_OFFSET_MINUTES = -720;
constexpr int MAX_UTC_OFFSET_MINUTES = 840;

// 將已驗證事件放進 UiTask 共用 Queue。
// event：const UiEvent&，UI 事件；回傳：bool，是否成功送入 Queue。
bool sendUiEvent(const UiEvent &event) {
    if (uiEventQueue == nullptr) {
        Serial.println("[MqttEventRouter] UI Queue 尚未建立。");
        return false;
    }
    if (xQueueSend(uiEventQueue, &event, 0) != pdTRUE) {
        Serial.println("[MqttEventRouter] UI Queue 已滿，捨棄 MQTT 事件。");
        return false;
    }
    return true;
}

// 解析韌體版本並建立 UI 事件。
// payload：const uint8_t*，原始內容；length：size_t，內容長度；回傳：bool，是否成功。
bool routeFirmwareUpdate(const uint8_t *payload, size_t length) {
    if (length == 0 || length >= APP_VERSION_LENGTH) {
        Serial.println("[MqttEventRouter] 韌體版本長度無效。");
        return false;
    }

    UiEvent event{};
    event.type = UiEventType::FIRMWARE_UPDATE;
    memcpy(event.firmware.version, payload, length);
    event.firmware.version[length] = '\0';
    return sendUiEvent(event);
}

// 解析後端傳來的精簡天氣 JSON 並建立 UI 事件。
// payload：const uint8_t*，JSON 內容；length：size_t，內容長度；回傳：bool，是否成功。
bool routeWeatherUpdate(const uint8_t *payload, size_t length) {
    StaticJsonDocument<WEATHER_JSON_CAPACITY> document;
    const DeserializationError error = deserializeJson(document, payload, length);
    if (error) {
        Serial.printf("[MqttEventRouter] 天氣 JSON 無效：%s\n", error.c_str());
        return false;
    }

    const float temperature = document["t"] | NAN;
    const int conditionCode = document["c"] | -1;
    const int rainProbability = document["p"] | -1;
    const int utcOffsetMinutes = document["z"] | INT16_MAX;
    const uint32_t observedAt = document["at"] | 0u;
    const int temperatureTenths = lroundf(temperature * 10.0f);
    if (!std::isfinite(temperature)
        || temperature < MIN_TEMPERATURE_C
        || temperature > MAX_TEMPERATURE_C
        || conditionCode < 1
        || conditionCode > UINT16_MAX
        || rainProbability < 0
        || rainProbability > 100
        || utcOffsetMinutes < MIN_UTC_OFFSET_MINUTES
        || utcOffsetMinutes > MAX_UTC_OFFSET_MINUTES
        || observedAt == 0
        || temperatureTenths < INT16_MIN
        || temperatureTenths > INT16_MAX) {
        Serial.println("[MqttEventRouter] 天氣欄位無效。");
        return false;
    }

    UiEvent event{};
    event.type = UiEventType::WEATHER_UPDATED;
    event.weather.temperatureTenthsC = static_cast<int16_t>(temperatureTenths);
    event.weather.conditionCode = static_cast<uint16_t>(conditionCode);
    event.weather.rainProbability = static_cast<uint8_t>(rainProbability);
    event.weather.utcOffsetMinutes = static_cast<int16_t>(utcOffsetMinutes);
    event.weather.observedAt = observedAt;
    if (!sendUiEvent(event)) return false;

    Serial.printf(
        "[Weather] UI event: %d.%d C, code=%u, rain=%u%%, UTC%+d min\n",
        event.weather.temperatureTenthsC / 10,
        abs(event.weather.temperatureTenthsC % 10),
        event.weather.conditionCode,
        event.weather.rainProbability,
        event.weather.utcOffsetMinutes
    );
    return true;
}
}

// 依 topic 將 MQTT 原始訊息轉成對應 UI 事件。
// topic：const char*，收到的主題；payload：const uint8_t*，內容；length：size_t，長度；weatherTopic：const char*，本機天氣主題。
// 回傳：bool，是否成功轉送。
bool MqttEventRouter_Route(
    const char *topic,
    const uint8_t *payload,
    size_t length,
    const char *weatherTopic
) {
    if (topic == nullptr || payload == nullptr || weatherTopic == nullptr) {
        return false;
    }
    if (strcmp(topic, MQTT_FIRMWARE_VERSION_TOPIC) == 0) {
        return routeFirmwareUpdate(payload, length);
    }
    if (strcmp(topic, weatherTopic) == 0) {
        return routeWeatherUpdate(payload, length);
    }

    Serial.printf("[MqttEventRouter] 未處理主題：%s\n", topic);
    return false;
}
