#pragma once

#include <freertos/FreeRTOS.h>

// 啟動唯一擁有 MQTT 連線的背景 Task。
void MqttTask_Start(UBaseType_t priority, BaseType_t core);

// 等待開機的首次 MQTT 連線嘗試完成，再開始載入大型地圖資料。
bool MqttTask_WaitForInitialSetup(uint32_t timeoutMs);

// 地圖完成大型記憶體配置後才要求掃描，避免 Wi-Fi 掃描切碎 heap。
void MqttTask_RequestBootWifiScan();

