#include "App/AppQueues.h"

#include "App/AppMessages.h"

QueueHandle_t mqttInboundQueue = nullptr;
QueueHandle_t mqttPublishQueue = nullptr;
QueueHandle_t uiCommandQueue = nullptr;
QueueHandle_t uiResponseQueue = nullptr;

// 建立 FreeRTOS 隊列，並檢查是否成功。
bool AppQueues_Create() {
    // 長度刻意不大：版本、狀態都是小訊息，滿了代表上游傳送過快，方便及早發現問題。
    mqttInboundQueue = xQueueCreate(8, sizeof(MqttInboundEvent));
    mqttPublishQueue = xQueueCreate(8, sizeof(MqttPublishRequest));
    uiCommandQueue   = xQueueCreate(8, sizeof(UiCommand));
    uiResponseQueue  = xQueueCreate(4, sizeof(UiResponse));

    return mqttInboundQueue != nullptr 
        && mqttPublishQueue != nullptr 
        && uiCommandQueue != nullptr 
        && uiResponseQueue != nullptr;
}
