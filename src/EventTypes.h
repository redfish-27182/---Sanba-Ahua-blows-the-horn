#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// TouchTask 傳送到 inputQueue 的觸控／手勢事件。
enum class InputEventType {
    TOUCH_DOWN,
    DRAG,
    TAP,
    LONG_PRESS,
    SWIPE,
    RELEASE
};

// 一筆觸控事件的座標與按壓資訊。
struct InputEvent {
    InputEventType type;
    int16_t x;
    int16_t y;
    int16_t startX;
    int16_t startY;
    uint32_t durationMs;
    uint32_t timestamp;
};

// 由 main.cpp 建立，供 TouchTask 與 PromptDialog 共用。
extern QueueHandle_t inputQueue;
