#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>

// 我自己寫的程式庫
#include "Config.h"
#include "EventTypes.h"
#include "UI/PromptDialog.h"
#include "UpdateManager/UpdateManager.h"
#include "Tasks/TouchTask.h"
#include "Tasks/DisplayTask.h"

// 開源程式庫宣告對象
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// 我自己封裝的程式庫
PromptDialog dialog(tft);
UpdateManager updateManager("http://192.168.0.101:8000/api/v1/config", "1.0.0", tft);
SystemConfig pendingConfig;

// FreeRTOS 隊列
QueueHandle_t inputQueue = nullptr;

void setup() {
    Serial.begin(115200);

    // 1. 螢幕初始化
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // 2. 初始化獨立觸控 SPI
    touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(touchSpi);
    ts.setRotation(1);

    constexpr UBaseType_t inputQueueLength = 10;
    inputQueue = xQueueCreate(inputQueueLength, sizeof(InputEvent));

    // PromptDialog 也使用 TouchTask 的事件，因此必須在首次 show() 前準備完成。
    if (inputQueue == nullptr) {
        Serial.println("Failed to create input queue.");
    } else {
        constexpr UBaseType_t touchTaskPriority = 4;
        TouchTask_Start(ts, touchTaskPriority, 1);
    }

    // 3. WiFi 連線 (Exit 按鈕 + 逾時跳過)
    WiFiManager wm;
    wm.setConnectTimeout(8); // 設定嘗試連線已存 Wi-Fi 的逾時時間 (8秒)
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Connecting to saved WiFi...");

    // 先嘗試自動連接已存的 WiFi (如果連不上，不要立刻開熱點卡死)
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

    // 4. 檢查更新 (完整保留 OTA 與對話框邏輯)
    if (isConnected) {
        Serial.println("✅ Wi-Fi 連線成功！檢查系統更新中...");
        
        if (updateManager.hasPendingUpdate(pendingConfig)) {
            String msg = "New Version Available!\nFW: " + pendingConfig.firmwareVersion + "\nUpdate now?";
            
            bool userChoice = dialog.show("SYSTEM UPDATE", msg, "[A] OK", "[B] CANCEL");

            if (userChoice) {
                SPI.begin();
                SD.begin(SD_CS_PIN);
                updateManager.executePendingUpdate(pendingConfig);
            } else {
                Serial.println("跳過更新，進入系統...");
            }
        }
    } else {
        Serial.println("❌ 進入離線模式 (跳過更新檢查)");
        WiFi.mode(WIFI_OFF);
    }

    tft.fillScreen(TFT_BLACK);

    // 5. 初始化 SD 卡
    SPI.begin();
    if (!SD.begin(SD_CS_PIN))  Serial.println("⚠️ SD 卡掛載失敗或未插入！");
    else                       Serial.println("✅ SD 卡載入完成");

    // 啟動用對話框已結束；之後由 DisplayTask 消費觸控事件並更新畫面。
    DisplayTask_Start(tft, ts, 3, 1);

    Serial.println("🚀 系統初始化完成，裝置啟動！");
}

void loop() {
    // 渲染與睡眠邏輯已移至 DisplayTask，主 loop 休眠防看門狗報警
    vTaskDelay(pdMS_TO_TICKS(1000));
}
