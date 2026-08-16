#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "UI/PromptDialog.h"
#include "UpdateManager/UpdateManager.h"

#define TFT_BL 21
#define SD_CS_PIN 5

// 🎯 完全使用你測試成功的腳位與物件名稱 ts
#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33

TFT_eSPI tft = TFT_eSPI();

SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

PromptDialog dialog(tft, ts); // 將 ts 傳給 dialog
UpdateManager updateManager("http://192.168.0.101:8000/api/v1/config", "1.0.0", tft);

SystemConfig pendingConfig;

// ==========================================
// 1. 觸控事件型別與資料結構
// ==========================================
enum class InputEventType {
    TOUCH_DOWN,
    DRAG,
    TAP,
    LONG_PRESS,
    SWIPE,
    RELEASE
};

enum class GestureCategory { NONE, TAP_TYPE, SWIPE_TYPE };
GestureCategory lastCategory = GestureCategory::NONE;

struct InputEvent {
    InputEventType type;
    int16_t x;
    int16_t y;
    int16_t startX;
    int16_t startY;
    uint32_t durationMs;
    uint32_t timestamp;
};

// 影子回放用軌跡節點
struct ReplayNode {
    InputEvent event;
    bool played;
};

QueueHandle_t inputQueue = nullptr;
const int MAX_REPLAY_POINTS = 120;
ReplayNode replayBuffer[MAX_REPLAY_POINTS];
int bufferHead = 0;
int lastReplayX = -1, lastReplayY = -1;

// ==========================================
// 2. Core 1: 觸控採樣、軟體濾波與手勢結算任務
// ==========================================
void touchTask(void *pvParameters) {
    bool isPressed = false;
    uint32_t pressStartTime = 0;
    int16_t startX = 0, startY = 0;
    int16_t lastX = 0, lastY = 0;

    for (;;) {
        if (ts.touched()) {
            TS_Point p1 = ts.getPoint();
            vTaskDelay(pdMS_TO_TICKS(5));
            TS_Point p2 = ts.getPoint();

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
                    // 按壓中（移動位移大於 5px 視為拖曳）
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

const unsigned long SLEEP_TIMEOUT_MS = 20000; // 20 秒無操作自動進入硬體睡眠
unsigned long lastActivityTime = 0;
bool lastTouched = false;

void setup() {
    Serial.begin(115200);
    delay(500);

    // 1. 螢幕初始化
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // 2. 🎯 完全比照你的成功測試程式：初始化獨立觸控 SPI
    touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(touchSpi);
    ts.setRotation(1);

    // 3. WiFi 連線
    WiFiManager wm;
    wm.autoConnect("ESP32-Console");

    // 4. 檢查更新與彈出對話框 (此時 SD 卡還沒 begin，SPI 極度乾淨！)
    if (updateManager.hasPendingUpdate(pendingConfig)) {
        String msg = "New Version Available!\nFW: " + pendingConfig.firmwareVersion + "\nUpdate now?";
        
        bool userChoice = dialog.show("SYSTEM UPDATE", msg, "[A] OK", "[B] CANCEL");

        if (userChoice) {
            // 使用者按了 OK 之後，才初始化 SD 卡並執行下載！
            SPI.begin();
            SD.begin(SD_CS_PIN);
            updateManager.executePendingUpdate(pendingConfig);
        } else {
            Serial.println("跳過更新，進入遊戲...");
        }
    }

    // 初始化 SD 卡（供遊戲讀取資源）
    SPI.begin();
    SD.begin(SD_CS_PIN);

    lastActivityTime = millis();
    inputQueue = xQueueCreate(10, sizeof(InputEvent));
    xTaskCreatePinnedToCore(touchTask, "TouchTask", 4096, NULL, 4, NULL, 1);
}

// 進入 ESP32 硬體淺度睡眠
void enterLightSleep() {
    digitalWrite(TFT_BL, LOW); // 關閉背光
    tft.fillScreen(TFT_BLACK); // 清空螢幕
    Serial.flush(); // 清空緩衝區
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0); // 設定 GPIO 36 為喚醒來源
    esp_light_sleep_start(); // 進入淺度睡眠
    digitalWrite(TFT_BL, HIGH); // 點亮背光

    // 防誤觸：等待手指離開螢幕，避免喚醒的第一下誤點擊遊戲按鈕
    delay(100);
    while (ts.touched()) {delay(10);}
    delay(100);

    lastActivityTime = millis(); // 重設計時器
}

// ==========================================
// 3. 主迴圈：接收 Queue 與 1 秒影子回放渲染
// ==========================================
void loop() {
    InputEvent incoming;

    // A. 非同步提取 Queue 事件並寫入延遲環狀緩衝區
    while (xQueueReceive(inputQueue, &incoming, 0) == pdTRUE) {
        lastActivityTime = millis(); // 保持活躍狀態，重設計時

        replayBuffer[bufferHead] = {incoming, false};
        bufferHead = (bufferHead + 1) % MAX_REPLAY_POINTS;

        // 即時串口監控
        Serial.printf("📥 [INPUT QUEUE] Type: %d | Pos: (%d, %d) | Start: (%d, %d) | Dur: %u ms\n",
                      static_cast<int>(incoming.type), incoming.x, incoming.y,
                      incoming.startX, incoming.startY, incoming.durationMs);
    }

    // B. 1 秒影子回放渲染邏輯
    uint32_t now = millis();
    for (int i = 0; i < MAX_REPLAY_POINTS; i++) {
        if (!replayBuffer[i].played && replayBuffer[i].event.timestamp > 0) {
            if (now - replayBuffer[i].event.timestamp >= 1000) { // 剛好延遲滿 1 秒
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
                        // 🎯 只要之前不是滑動類（例如上一個是點擊），就清屏切換
                        if (lastCategory != GestureCategory::SWIPE_TYPE) {
                            tft.fillScreen(TFT_BLACK);
                            lastCategory = GestureCategory::SWIPE_TYPE;
                        }
                        // 繪製連續軌跡（青藍色）
                        if (lastReplayX >= 0 && lastReplayY >= 0) {
                            tft.drawLine(lastReplayX, lastReplayY, evt.x, evt.y, TFT_CYAN);
                        }
                        tft.fillCircle(evt.x, evt.y, 4, TFT_CYAN);
                        lastReplayX = evt.x;
                        lastReplayY = evt.y;
                        break;

                    case InputEventType::SWIPE:
                        // 🎯 確保滑動直線繪製時為滑動類別
                        if (lastCategory != GestureCategory::SWIPE_TYPE) {
                            tft.fillScreen(TFT_BLACK);
                            lastCategory = GestureCategory::SWIPE_TYPE;
                        }
                        tft.drawLine(evt.startX, evt.startY, evt.x, evt.y, TFT_YELLOW);
                        tft.fillCircle(evt.x, evt.y, 8, TFT_YELLOW);
                        lastReplayX = -1;
                        lastReplayY = -1;
                        break;

                    case InputEventType::TAP:
                        // 🎯 只要之前不是點擊類（例如上一個是滑動），就清屏切換
                        if (lastCategory != GestureCategory::TAP_TYPE) {
                            tft.fillScreen(TFT_BLACK);
                            lastCategory = GestureCategory::TAP_TYPE;
                        }
                        tft.fillCircle(evt.x, evt.y, 12, TFT_GREEN);
                        tft.drawCircle(evt.x, evt.y, 16, TFT_WHITE);
                        lastReplayX = -1;
                        lastReplayY = -1;
                        break;

                    case InputEventType::LONG_PRESS:
                        if (lastCategory != GestureCategory::TAP_TYPE) {
                            tft.fillScreen(TFT_BLACK);
                            lastCategory = GestureCategory::TAP_TYPE;
                        }
                        tft.fillCircle(evt.x, evt.y, 8, TFT_MAGENTA);
                        tft.drawCircle(evt.x, evt.y, 18, TFT_RED);
                        tft.drawCircle(evt.x, evt.y, 24, TFT_YELLOW);
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

    delay(10); // 維持約 100 FPS 的輪詢與畫面刷新
}