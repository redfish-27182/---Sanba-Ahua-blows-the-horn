#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>

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
SPIClass touchSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ); // 確定使用 ts

PromptDialog dialog(tft, ts); // 將 ts 傳給 dialog
UpdateManager updateManager("http://192.168.0.101:8000/api/v1/config", "1.0.0", tft);

SystemConfig pendingConfig;

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
}

void loop() {
}