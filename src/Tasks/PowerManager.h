#pragma once

#include <Arduino.h>

/**
 * 管理螢幕背光與 ESP32 的閒置休眠。
 *
 * TouchTask 在確認觸控有效後呼叫 PowerManager_HandleTouchActivity()。
 * 若螢幕原本已熄滅，該函式會喚醒背光並回傳 true；呼叫端應忽略這整次
 * 觸碰，讓使用者下一次觸碰才操作 UI。
 */

// 啟動 PowerManager 背光與休眠管理工作。
void PowerManager_Start(UBaseType_t priority, BaseType_t coreID);

// 觸控事件發生時呼叫，更新閒置時間。
bool PowerManager_HandleTouchActivity();
