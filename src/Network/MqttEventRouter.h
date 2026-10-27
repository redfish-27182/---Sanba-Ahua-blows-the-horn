#pragma once

#include <Arduino.h>

// 將已訂閱的 MQTT 原始訊息轉成 UiTask 可處理的固定大小事件。
// topic：const char*，收到的主題；payload：const uint8_t*，原始內容；length：size_t，內容長度；weatherTopic：const char*，本機天氣主題。
// 回傳：bool，是否成功辨識、驗證並轉送事件。
bool MqttEventRouter_Route(
    const char *topic,
    const uint8_t *payload,
    size_t length,
    const char *weatherTopic
);
