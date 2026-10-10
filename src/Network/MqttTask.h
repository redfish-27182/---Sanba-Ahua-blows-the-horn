#pragma once

#include <freertos/FreeRTOS.h>

// 啟動唯一擁有 MQTT 連線的背景 Task。
void MqttTask_Start(UBaseType_t priority, BaseType_t core);
