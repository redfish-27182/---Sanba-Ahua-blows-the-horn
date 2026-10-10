#include "TouchTask.h"
#include "../Config.h"
#include "../EventTypes.h"


static XPT2046_Touchscreen *pTs = nullptr; // 指向呼叫端建立的觸控物件；該物件的生命週期必須長於此背景工作。

// FreeRTOS 工作本體：讀取觸控資料、消除雜訊，並轉換成輸入事件。
static void touchTaskCode(void *pvParameters) {
    bool isPressed = false;           // 目前是否處於按壓狀態
    uint32_t pressStartTime = 0;      // 觸控按下的時間戳記
    int16_t  startX = 0, startY = 0;  // 觸控按下的起始位置
    int16_t  lastX = 0,  lastY = 0;   // 觸控按下後最後一次可靠位置

    for (;;) {
        if (pTs->touched()) {
            // 連續取樣兩次；兩次差太多時視為雜訊，不產生事件。
            TS_Point p1 = pTs->getPoint();
            vTaskDelay(pdMS_TO_TICKS(5));
            TS_Point p2 = pTs->getPoint();

            // 原始值在 5ms 內變動超過 200，視為雜訊。
            if (abs(p1.x - p2.x) < 200 && abs(p1.y - p2.y) < 200) {
                // 取兩次樣本的平均，再以此觸控面板的校正範圍映射至 320 x 240 螢幕。
                int16_t curX = map((p1.x + p2.x) / 2, 200, 3700, 0, 320);
                int16_t curY = map((p1.y + p2.y) / 2, 240, 3800, 0, 240);
                curX = constrain(curX, 0, 319);
                curY = constrain(curY, 0, 239);

                uint32_t now = millis();

                if (!isPressed) {
                    // 偵測到本次按壓的第一個可靠樣本：記錄起點並通知畫面端
                    isPressed = true;
                    pressStartTime = now;
                    startX = curX;
                    startY = curY;
                    lastX = curX;
                    lastY = curY;
                    
                    // 傳出evt數據到FreeRTOS隊列
                    InputEvent evt = {InputEventType::TOUCH_DOWN, curX, curY, startX, startY, 0, now};
                    xQueueSend(inputQueue, &evt, 0);
                    
                } else {
                    // 同一次按壓中，位移超過 4px 才回報 DRAG
                    if (abs(curX - lastX) > 4 || abs(curY - lastY) > 4) {
                        // 傳出evt數據到FreeRTOS隊列
                        InputEvent evt = {InputEventType::DRAG, curX, curY, startX, startY, now - pressStartTime, now};
                        xQueueSend(inputQueue, &evt, 0);
                        lastX = curX;
                        lastY = curY;
                    }
                }
            }
        } else {
            if (isPressed) {
                // 手指離開面板：以停留時間與「起點到最後位置」的距離判定手勢。
                uint32_t now = millis();
                uint32_t dur = now - pressStartTime;
                int16_t totalDistX = lastX - startX;
                int16_t totalDistY = lastY - startY;
                int16_t totalDist = sqrt(totalDistX * totalDistX + totalDistY * totalDistY);

                // 無符合特定手勢時，預設只回報一般放開事件。
                InputEventType finalType = InputEventType::RELEASE;

                // 判斷順序很重要：短距離、短時間是點擊；短距離、長時間是長按；
                // 夠遠且夠快才是滑動。其他組合保留為 RELEASE。
                if (totalDist < 15 && dur <= 300) {
                    finalType = InputEventType::TAP;
                } else if (totalDist < 15 && dur > 600) {
                    finalType = InputEventType::LONG_PRESS;
                } else if (totalDist >= 35 && dur < 400) {
                    finalType = InputEventType::SWIPE;
                }

                InputEvent evt = {finalType, lastX, lastY, startX, startY, dur, now};
                xQueueSend(inputQueue, &evt, 0);
                // 重設狀態，讓下次觸碰可重新送出 TOUCH_DOWN。
                isPressed = false;
            }
        }

        // 每 12ms 輪詢一次，約為 80Hz；避免此工作佔滿 CPU。
        vTaskDelay(pdMS_TO_TICKS(12));
    }
}

// FreeRTOS 工作建立函式
void TouchTask_Start(
    XPT2046_Touchscreen &touchscreen, 
    UBaseType_t priority, // FreeRTOS 優先權
    BaseType_t coreID     // ESP32 核心編號
) {
    // 保存呼叫端建立的物件；該物件的生命週期必須長於此背景工作。
    pTs = &touchscreen;
    // 配置 4096 bytes 堆疊，並固定到指定核心；工作會持續執行，不會自行結束。
    xTaskCreatePinnedToCore(touchTaskCode, "TouchTask", 4096, NULL, priority, NULL, coreID);
}
