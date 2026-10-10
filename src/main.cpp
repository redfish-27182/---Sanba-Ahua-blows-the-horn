#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>

// 我自己寫的程式庫
#include "Config.h"
#include "EventTypes.h"
#include "App/AppQueues.h"
#include "UI/PromptDialog.h"
#include "UI/UiTask.h"
#include "Network/MqttTask.h"
#include "UpdateManager/UpdateManager.h"
#include "Tasks/TouchTask.h"

// 開源程式庫宣告對象
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// 我自己封裝的程式庫
PromptDialog dialog(tft);

// FreeRTOS 隊列
QueueHandle_t inputQueue = nullptr;

void setup() {
    Serial.begin(115200);

    // --------------------1. 螢幕初始化 --------------------------//

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // -------------------2. 初始化獨立觸控 SPI -------------------//

    touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(touchSpi);
    ts.setRotation(1);

    // -------------------3. 建立 FreeRTOS 隊列 -------------------//

    constexpr UBaseType_t inputQueueLength = 10; // FreeRTOS 隊列長度
    inputQueue = xQueueCreate(inputQueueLength, sizeof(InputEvent));
    const bool appQueuesReady = AppQueues_Create();

    // -------------------4. 啟動 TouchTask -------------------//

    if (inputQueue == nullptr) {
        Serial.println("Failed to create input queue.");
    } else {
        constexpr UBaseType_t touchTaskPriority = 4;
        TouchTask_Start(ts, touchTaskPriority, 1); 
    }

    // -----------------5. WiFi 連線 (Exit 按鈕 + 逾時跳過) -----------------//
    WiFiManager wm;
    wm.setConnectTimeout(8);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Connecting to saved WiFi...");
    tft.setTextSize(1);
    
    bool isConnected = wm.autoConnect();

        // 如果沒連上已存的 WiFi，彈出對話框詢問使用者
    if (!isConnected) {
        Serial.println("⚠️ 無法連線至已儲存的 Wi-Fi");

        // 呼叫PromptDialog 選擇框
        String wifiMsg = "Failed to connect WiFi.\nOpen AP Config Portal\nto configure network?";
        bool wantConfig = dialog.show("WIFI SETUP", wifiMsg, "[A] CONFIG", "[B] OFFLINE");

        if (wantConfig) {
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.setTextSize(1);
            tft.setCursor(10, 10);
            tft.println("AP: ESP32-Console");
            tft.println("Connecting from phone...");
            tft.println("(Tap 'Exit' on phone or wait to cancel)");

            wm.setConfigPortalTimeout(60); // 60 秒逾時
            isConnected = wm.startConfigPortal("ESP32-Console");

            // 🎯 如果在手機點了 Exit 或 60 秒沒配網成功
            if (!isConnected) {
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_RED, TFT_BLACK);
                tft.setCursor(10, 10);
                tft.println("WiFi Setup Canceled / Timeout.");
                tft.println("Starting in Offline Mode...");
                delay(1200); // 提示 1.2 秒
            }
        }else {
            Serial.println("使用者選擇離線模式");
        }
    }
    
    tft.fillScreen(TFT_BLACK);

    // -------------------6. 初始化 SD 卡 -------------------//
    SPI.begin();
    if (!SD.begin(SD_CS_PIN))  Serial.println("⚠️ SD 卡掛載失敗或未插入！");
    else                       Serial.println("✅ SD 卡載入完成");

    // -------------------7. 啟動其他 Task -------------------//
    if (!appQueuesReady) {
        Serial.println("Failed to create application queues.");
    } else {
        UiTask_Start(tft, dialog, 3, 1); // UiTask，啟動!!!!!
        UpdateManager_Start(tft, 2, 0);  // UpdateManager，啟動!!!!!
        MqttTask_Start(2, 0);            // MqttTask，啟動!!!!!
    }

    Serial.println("🚀 系統初始化完成，原神啟動！");
}

void loop() {
    // MQTT、更新協調與正常 UI 都已移至各自 Task；主 loop 保持閒置。
    vTaskDelay(pdMS_TO_TICKS(1000));
}
