#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class InputEventType {
    TOUCH_DOWN,
    DRAG,
    TAP,
    LONG_PRESS,
    SWIPE,
    RELEASE
};

enum class GestureCategory {
    NONE,
    TAP_TYPE,
    SWIPE_TYPE
};

struct InputEvent {
    InputEventType type;
    int16_t x;
    int16_t y;
    int16_t startX;
    int16_t startY;
    uint32_t durationMs;
    uint32_t timestamp;
};

struct ReplayNode {
    InputEvent event;
    bool played;
};

// 全域隊列控制把手 (extern 讓 Tasks 共用)
extern QueueHandle_t inputQueue;