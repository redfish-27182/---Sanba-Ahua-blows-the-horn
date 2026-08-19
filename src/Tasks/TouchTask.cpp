#include "TouchTask.h"
#include "../Config.h"
#include "../EventTypes.h"

static XPT2046_Touchscreen *pTs = nullptr;

static void touchTaskCode(void *pvParameters) {
    bool isPressed = false;
    uint32_t pressStartTime = 0;
    int16_t startX = 0, startY = 0;
    int16_t lastX = 0, lastY = 0;

    for (;;) {
        if (pTs->touched()) {
            TS_Point p1 = pTs->getPoint();
            vTaskDelay(pdMS_TO_TICKS(5));
            TS_Point p2 = pTs->getPoint();

            // 雙重採樣軟體濾波（抗電阻屏跳點雜訊）
            if (abs(p1.x - p2.x) < 200 && abs(p1.y - p2.y) < 200) {
                int16_t curX = map((p1.x + p2.x) / 2, 200, 3700, 0, 320);
                int16_t curY = map((p1.y + p2.y) / 2, 240, 3800, 0, 240);
                curX = constrain(curX, 0, 319);
                curY = constrain(curY, 0, 239);

                uint32_t now = millis();

                if (!isPressed) {
                    // 手指剛按下瞬間
                    isPressed = true;
                    pressStartTime = now;
                    startX = curX;
                    startY = curY;
                    lastX = curX;
                    lastY = curY;

                    InputEvent evt = {InputEventType::TOUCH_DOWN, curX, curY, startX, startY, 0, now};
                    xQueueSend(inputQueue, &evt, 0);
                } else {
                    // 按壓中（位移大於 4px 視為拖曳）
                    if (abs(curX - lastX) > 4 || abs(curY - lastY) > 4) {
                        InputEvent evt = {InputEventType::DRAG, curX, curY, startX, startY, now - pressStartTime, now};
                        xQueueSend(inputQueue, &evt, 0);
                        lastX = curX;
                        lastY = curY;
                    }
                }
            }
        } else {
            if (isPressed) {
                // 手指放開，進行手勢結算
                uint32_t now = millis();
                uint32_t dur = now - pressStartTime;
                int16_t totalDistX = lastX - startX;
                int16_t totalDistY = lastY - startY;
                int16_t totalDist = sqrt(totalDistX * totalDistX + totalDistY * totalDistY);

                InputEventType finalType = InputEventType::RELEASE;

                if (totalDist < 15 && dur <= 300) {
                    finalType = InputEventType::TAP;
                } else if (totalDist < 15 && dur > 600) {
                    finalType = InputEventType::LONG_PRESS;
                } else if (totalDist >= 35 && dur < 400) {
                    finalType = InputEventType::SWIPE;
                }

                InputEvent evt = {finalType, lastX, lastY, startX, startY, dur, now};
                xQueueSend(inputQueue, &evt, 0);
                isPressed = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(12)); // 穩定輪詢週期約 80Hz
    }
}

void TouchTask_Start(XPT2046_Touchscreen &touchscreen, UBaseType_t priority, BaseType_t coreID) {
    pTs = &touchscreen;
    xTaskCreatePinnedToCore(touchTaskCode, "TouchTask", 4096, NULL, priority, NULL, coreID);
}