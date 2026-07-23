#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>

#include "FastAPIClient.h"
#include "GitHubOTA.h"

#define TFT_BL 21
const char* CURRENT_VERSION = "v0.0.0";

const char* ssid = "michael";
const char* password = "0932749747";

// 1. 建立普通物件
TFT_eSPI tft = TFT_eSPI();

// 2. 直接傳入普通變數，完全不用指標！
FastAPIClient apiServer("http://192.168.0.101:8000"); // 填入你的 FastAPI 位址
GitHubOTA otaEngine(tft);                              // 🎯 直接把 tft 傳進去！

void setup() {
    Serial.begin(115200);

    // 硬體初始化
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // 連接 Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ Wi-Fi 已連線！");

    // 步驟 1：查詢版本
    UpdateInfo info;
    if (apiServer.checkUpdate(CURRENT_VERSION, info)) {
        Serial.printf("📢 伺服器訊息: %s\n", info.message.c_str());

        // 步驟 2：執行 OTA 升級
        if (info.has_update) {
            Serial.println("✨ 發現新版本，啟動 OTA 升級流程...");
            otaEngine.startOTA(info.download_url);
        } else {
            Serial.println("✅ 已是最新版本。");
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("SYSTEM UP TO DATE", 160, 120, 4);
        }
    }
}

void loop() {
    delay(1000);
}