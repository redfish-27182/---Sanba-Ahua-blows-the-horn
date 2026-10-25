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
#include "Tasks/TouchTask.h"
#include "Tasks/PowerManager.h"
#include "Font/SystemText.h"

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

    // -----------------2. SD 卡初始化/載入字庫 ------------------------//
    SPI.begin();
    if (!SD.begin(SD_CS_PIN)) Serial.println("⚠️ SD 卡掛載失敗或未插入！");
    else if (!SystemText_Load()) Serial.println("⚠️ 系統字庫載入失敗，改用英文提示。");
    else Serial.println("✅ SD 卡及系統字庫載入完成");

    // -------------------3. 初始化獨立觸控 SPI -------------------//

    touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(touchSpi);
    ts.setRotation(1);

    // -------------------4. 建立 FreeRTOS 隊列 -------------------//

    constexpr UBaseType_t inputQueueLength = 10; // FreeRTOS 隊列長度
    inputQueue = xQueueCreate(inputQueueLength, sizeof(InputEvent));
    const bool appQueuesReady = AppQueues_Create();

    // -------------------5. 啟動 TouchTask -------------------//

    if (inputQueue == nullptr) {
        Serial.println("Failed to create input queue.");
    } else {
        constexpr UBaseType_t touchTaskPriority = 4;
        TouchTask_Start(ts, touchTaskPriority, 1); 
    }

    // -------------------6. WiFi 連線 (Exit 按鈕 + 逾時跳過) -----------------//
    WiFiManager wm;
    wm.setConnectTimeout(8);
    
    SystemText_Draw(tft, "正在連線 WiFi...", "Connecting to saved WiFi...", 10, 10, 300, 40, TFT_WHITE);
    
    bool isConnected = wm.autoConnect();

        // 如果沒連上已存的 WiFi，彈出對話框詢問使用者
    if (!isConnected) {
        Serial.println("⚠️ 無法連線至已儲存的 Wi-Fi");

        // 呼叫PromptDialog 選擇框
        String wifiMsg = SystemText_Choose("WiFi 連線失敗\n要設定網路嗎？", "WiFi failed.\nConfigure network?");
        bool wantConfig = dialog.show(
            SystemText_Choose("WiFi 設定", "WIFI SETUP"), 
            wifiMsg,
            SystemText_Choose("設定", "CONFIG"), 
            SystemText_Choose("離線", "OFFLINE")
        );

        if (wantConfig) {
            tft.fillScreen(TFT_BLACK);
            SystemText_Draw(tft, "AP: ESP32-Console\n請用手機連線設定\n手機按 Exit 或等待取消", "AP: ESP32-Console\nConnect from phone\nTap Exit or wait to cancel", 10, 10, 300, 100, TFT_YELLOW);

            wm.setConfigPortalTimeout(60); // 60 秒逾時
            isConnected = wm.startConfigPortal("ESP32-Console");

            // 🎯 如果在手機點了 Exit 或 60 秒沒配網成功
            if (!isConnected) {
                tft.fillScreen(TFT_BLACK);
                SystemText_Draw(tft, "WiFi 設定取消或逾時\n正在進入離線模式...", "WiFi setup canceled or timed out.\nStarting offline...", 10, 10, 300, 60, TFT_RED);
                delay(1200); // 提示 1.2 秒
            }
        }else {
            Serial.println("使用者選擇離線模式");
        }
    }

    tft.fillScreen(TFT_BLACK);

    // -------------------7. 啟動其他 Task -------------------//
    if (!appQueuesReady) {
        Serial.println("Failed to create application queues.");
    } else {
        tft.fillScreen(TFT_BLACK);
        SystemText_Draw(tft, "正在連線 MQTT...", "Connecting MQTT...", 10, 100, 300, 24, TFT_WHITE, true);
        MqttTask_Start(2, 0);            // MqttTask，啟動!!!!!
        if (WiFi.status() == WL_CONNECTED && !MqttTask_WaitForInitialSetup(20000)) {
            Serial.println("[MQTT] Initial setup timed out; starting the game anyway.");
        }
        UiTask_Start(tft, dialog, 3, 1); // UiTask，啟動!!!!!
    }

    // TouchTask 會在每次有效觸碰時更新閒置時間。
    // 短暫閒置只關閉背光；長時間閒置才進入 deep sleep。
    PowerManager_Start(1, 1);

    Serial.println("🚀 系統初始化完成，原神啟動！");
}

void loop() {
    // MQTT 與 UI 分別執行；更新流程由 UI Task 處理，主 loop 保持閒置。
    vTaskDelay(pdMS_TO_TICKS(1000));
}
