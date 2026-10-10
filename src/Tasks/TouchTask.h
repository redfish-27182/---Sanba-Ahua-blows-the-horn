#pragma once
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>

/**
 * 啟動觸控讀取的 FreeRTOS 工作。
 *
 * 工作會持續讀取 @p touchscreen，將原始觸控座標轉成 320 x 240 螢幕座標，
 * 並經由全域 @c inputQueue 發送觸控與手勢事件。呼叫前必須先建立
 * @c inputQueue，而且 @p touchscreen 必須在工作執行期間持續有效。
 *
 * @param touchscreen 已初始化的 XPT2046 觸控控制器。
 * @param priority    此工作在 FreeRTOS 中的優先權。
 * @param coreID      要將工作固定執行的 CPU 核心編號（ESP32）。
 */

void TouchTask_Start(XPT2046_Touchscreen &touchscreen, UBaseType_t priority, BaseType_t coreID);
