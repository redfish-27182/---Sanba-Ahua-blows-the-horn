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

SPIClass touchSpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

PromptDialog dialog(tft, ts); // 將 ts 傳給 dialog
UpdateManager updateManager("http://192.168.0.101:8000/api/v1/config", "1.0.0", tft);

SystemConfig pendingConfig;

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

void loop() {
    // 1. 偵測正常觸控
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        int x = map(p.x, 200, 3700, 0, 320);
        int y = map(p.y, 240, 3800, 0, 240);

        Serial.printf("👉 [TOUCH] X: %d, Y: %d\n", x, y);

        // 重設計時器
        lastActivityTime = millis();
        delay(50);
    }

    // 2. 檢查是否超時
    if (millis() - lastActivityTime > SLEEP_TIMEOUT_MS) {
        enterLightSleep();
    }

    // 3. 正常遊戲主邏輯更新
    // ...

    delay(20);
}

