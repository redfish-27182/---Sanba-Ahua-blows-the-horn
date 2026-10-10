#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// 新架構的跨 Task Queue。inputQueue 是既有觸控 Queue，仍由原本程式維持。
extern QueueHandle_t mqttInboundQueue;
extern QueueHandle_t mqttPublishQueue;
extern QueueHandle_t uiCommandQueue;
extern QueueHandle_t uiResponseQueue;

// 在建立 Task 前呼叫；任一 Queue 建立失敗會回傳 false。
bool AppQueues_Create();
