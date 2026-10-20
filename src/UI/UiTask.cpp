#include "UI/UiTask.h"
#include "App/AppMessages.h"
#include "App/AppQueues.h"
#include "Display/DisplayDriver.h"
#include "EventTypes.h"
#include "UI/SceneManager.h"
#include "UpdateManager/UpdateManager.h"

namespace {
struct UiTaskContext { TFT_eSPI *tft; PromptDialog *dialog; };

// 執行畫面、觸控與 OTA 流程；對話框及 OTA 執行時遊戲自然暫停。
// parameter：void*，啟動時傳入的 UiTaskContext；無回傳值。
void uiTask(void *parameter) {
    const UiTaskContext context = *static_cast<UiTaskContext *>(parameter);
    delete static_cast<UiTaskContext *>(parameter);
    DisplayDriver display(*context.tft);
    SceneManager scenes(display);
    scenes.startGame();
    // 載入期間累積的觸控不應觸發遊戲。
    InputEvent input{};
    while (inputQueue && xQueueReceive(inputQueue, &input, 0) == pdTRUE) {}
    for (;;) {
        MqttInboundEvent event{};
        if (xQueueReceive(mqttInboundQueue, &event, 0) == pdTRUE) {
            if (UpdateManager_Handle(event, *context.tft, *context.dialog)) {
                while (inputQueue && xQueueReceive(inputQueue, &input, 0) == pdTRUE) {}
                scenes.redraw();
            }
        }
        UiCommand command{};
        // 現在狀態指令仍接收，但遊戲畫面暫不顯示連線狀態。
        while (xQueueReceive(uiCommandQueue, &command, 0) == pdTRUE) {}
        // 有界處理事件，避免快速觸控造成畫面完全無法更新。
        for (int i = 0; inputQueue && i < 10 && xQueueReceive(inputQueue, &input, 0) == pdTRUE; ++i) scenes.handleInput(input);
        scenes.update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
}

// 啟動包含遊戲與 OTA 流程的 UI Task。
// tft：TFT_eSPI&，螢幕；dialog：PromptDialog&，提示視窗；priority：UBaseType_t，優先權；core：BaseType_t，核心。
void UiTask_Start(
    TFT_eSPI &tft, 
    PromptDialog &dialog, 
    UBaseType_t priority, 
    BaseType_t core
) {
    auto *context = new UiTaskContext{&tft, &dialog};
    xTaskCreatePinnedToCore(
        uiTask, 
        "UiTask", 
        12288, 
        context, 
        priority, 
        nullptr, 
        core
    );
}
