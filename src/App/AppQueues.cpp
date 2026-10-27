#include "App/AppQueues.h"

#include "App/AppMessages.h"

QueueHandle_t uiEventQueue = nullptr;
QueueHandle_t mqttPublishQueue = nullptr;

// 建立 FreeRTOS 隊列，並檢查是否成功。
bool AppQueues_Create() {
    // 所有需要 UiTask 處理的 MQTT 事件共用此 Queue，不依資料種類拆分。
    uiEventQueue = xQueueCreate(8, sizeof(UiEvent));
    mqttPublishQueue = xQueueCreate(8, sizeof(MqttPublishRequest));

    return uiEventQueue != nullptr
        && mqttPublishQueue != nullptr;
}
