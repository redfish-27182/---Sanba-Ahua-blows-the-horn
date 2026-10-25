#pragma once

#include <freertos/FreeRTOS.h>

// 啟動唯一擁有 MQTT 連線的背景 Task。
void MqttTask_Start(UBaseType_t priority, BaseType_t core);

// 等待首次連線嘗試完成，讓 TLS 握手先於大型地圖資料載入。
bool MqttTask_WaitForInitialConnection(uint32_t timeoutMs);
