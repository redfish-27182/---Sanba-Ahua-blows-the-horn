#include "DisplayTask.h"
#include "../Config.h"
#include "../EventTypes.h"

static TFT_eSPI *pTft = nullptr;
static XPT2046_Touchscreen *pTs = nullptr;

static const int MAX_REPLAY_POINTS = 120;
static ReplayNode replayBuffer[MAX_REPLAY_POINTS];
static int bufferHead = 0;
static int lastReplayX = -1, lastReplayY = -1;
static GestureCategory lastCategory = GestureCategory::NONE;

static unsigned long lastActivityTime = 0;

static void enterLightSleep() {
    digitalWrite(TFT_BL, LOW);
    pTft->fillScreen(TFT_BLACK);
    Serial.flush();
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0);
    esp_light_sleep_start();
    digitalWrite(TFT_BL, HIGH);

    delay(100);
    while (pTs->touched()) {
        delay(10);
    }
    delay(100);

    lastActivityTime = millis();
}

static void displayTaskCode(void *pvParameters) {
    lastActivityTime = millis();
    InputEvent incoming;

    for (;;) {
        // A. 非同步提取 Queue 事件並寫入延遲環狀緩衝區
        while (xQueueReceive(inputQueue, &incoming, 0) == pdTRUE) {
            lastActivityTime = millis();

            replayBuffer[bufferHead] = {incoming, false};
            bufferHead = (bufferHead + 1) % MAX_REPLAY_POINTS;

            Serial.printf("📥 [INPUT QUEUE] Type: %d | Pos: (%d, %d) | Start: (%d, %d) | Dur: %u ms\n",
                          static_cast<int>(incoming.type), incoming.x, incoming.y,
                          incoming.startX, incoming.startY, incoming.durationMs);
        }

        // B. 1 秒影子回放渲染邏輯
        uint32_t now = millis();
        for (int i = 0; i < MAX_REPLAY_POINTS; i++) {
            if (!replayBuffer[i].played && replayBuffer[i].event.timestamp > 0) {
                if (now - replayBuffer[i].event.timestamp >= 1000) {
                    replayBuffer[i].played = true;
                    InputEvent evt = replayBuffer[i].event;

                    Serial.printf("👻 [GHOST REPLAY 1s] Executing Type: %d at (%d, %d)\n",
                                  static_cast<int>(evt.type), evt.x, evt.y);

                    switch (evt.type) {
                        case InputEventType::TOUCH_DOWN:
                            lastReplayX = evt.x;
                            lastReplayY = evt.y;
                            break;

                        case InputEventType::DRAG:
                            if (lastCategory != GestureCategory::SWIPE_TYPE) {
                                pTft->fillScreen(TFT_BLACK);
                                lastCategory = GestureCategory::SWIPE_TYPE;
                            }
                            if (lastReplayX >= 0 && lastReplayY >= 0) {
                                pTft->drawLine(lastReplayX, lastReplayY, evt.x, evt.y, TFT_CYAN);
                            }
                            pTft->fillCircle(evt.x, evt.y, 4, TFT_CYAN);
                            lastReplayX = evt.x;
                            lastReplayY = evt.y;
                            break;

                        case InputEventType::SWIPE:
                            if (lastCategory != GestureCategory::SWIPE_TYPE) {
                                pTft->fillScreen(TFT_BLACK);
                                lastCategory = GestureCategory::SWIPE_TYPE;
                            }
                            pTft->drawLine(evt.startX, evt.startY, evt.x, evt.y, TFT_YELLOW);
                            pTft->fillCircle(evt.x, evt.y, 8, TFT_YELLOW);
                            lastReplayX = -1;
                            lastReplayY = -1;
                            break;

                        case InputEventType::TAP:
                            if (lastCategory != GestureCategory::TAP_TYPE) {
                                pTft->fillScreen(TFT_BLACK);
                                lastCategory = GestureCategory::TAP_TYPE;
                            }
                            pTft->fillCircle(evt.x, evt.y, 12, TFT_GREEN);
                            pTft->drawCircle(evt.x, evt.y, 16, TFT_WHITE);
                            lastReplayX = -1;
                            lastReplayY = -1;
                            break;

                        case InputEventType::LONG_PRESS:
                            if (lastCategory != GestureCategory::TAP_TYPE) {
                                pTft->fillScreen(TFT_BLACK);
                                lastCategory = GestureCategory::TAP_TYPE;
                            }
                            pTft->fillCircle(evt.x, evt.y, 8, TFT_MAGENTA);
                            pTft->drawCircle(evt.x, evt.y, 18, TFT_RED);
                            pTft->drawCircle(evt.x, evt.y, 24, TFT_YELLOW);
                            lastReplayX = -1;
                            lastReplayY = -1;
                            break;

                        case InputEventType::RELEASE:
                            lastReplayX = -1;
                            lastReplayY = -1;
                            break;
                    }
                }
            }
        }

        // C. 檢查逾時進入睡眠
        if (millis() - lastActivityTime > SLEEP_TIMEOUT_MS) {
            enterLightSleep();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 維持約 100 FPS
    }
}

void DisplayTask_Start(TFT_eSPI &display, XPT2046_Touchscreen &touchscreen, UBaseType_t priority, BaseType_t coreID) {
    pTft = &display;
    pTs = &touchscreen;
    xTaskCreatePinnedToCore(displayTaskCode, "DisplayTask", 4096, NULL, priority, NULL, coreID);
}