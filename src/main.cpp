#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>

#include "Config.h"
#include "EventTypes.h"
#include "UI/PromptDialog.h"
#include "UpdateManager/UpdateManager.h"
#include "Tasks/TouchTask.h"
#include "Tasks/DisplayTask.h"

// 實例化全域硬體與隊列
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

PromptDialog dialog(tft, ts);
UpdateManager updateManager("http://192.168.0.101:8000/api/v1/config", "1.0.0", tft);
SystemConfig pendingConfig;

QueueHandle_t inputQueue = nullptr;

void setup() {
    Serial.begin(115200);
    delay(500);

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

    // 3. WiFi 連線 (Exit 按鈕 + 逾時跳過)
    WiFiManager wm;
    std::vector<const char *> menu = {"wifi", "info", "exit"};
    wm.setMenu(menu);
    wm.setConfigPortalTimeout(15);
    wm.setConnectTimeout(10);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 10);
    tft.println("Connecting WiFi / AP: ESP32-Console ...");

    bool isConnected = wm.autoConnect("ESP32-Console");

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
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("⚠️ SD 卡掛載失敗或未插入！");
    } else {
        Serial.println("✅ SD 卡載入完成");
    }

    // 6. 建立 FreeRTOS 隊列與啟動任務
    inputQueue = xQueueCreate(10, sizeof(InputEvent));

    // Core 1: 觸控採樣與手勢結算任務
    TouchTask_Start(ts, 4, 1);

    // Core 1: 畫面渲染與睡眠管理任務 (取代原本阻塞的 loop)
    DisplayTask_Start(tft, ts, 3, 1);

    Serial.println("🚀 系統初始化完成，裝置啟動！");
}

void loop() {
    // 渲染與睡眠邏輯已移至 DisplayTask，主 loop 休眠防看門狗報警
    vTaskDelay(pdMS_TO_TICKS(1000));
}