#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFiManager.h>

#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();

void initScreen() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1); // 橫屏 320x240
}

void setup() {
    Serial.begin(115200);
    delay(500);

    initScreen();

    // 繪製初始背景 (深藍色)
    tft.fillScreen(TFT_NAVY);

    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.drawString("ESP32 ARCADE SYSTEM", 10, 15, 4); // 使用 Font 4 大字體

    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("Connecting WiFi...", 10, 60, 2);

    // 🌐 啟動 WiFiManager
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    bool res = wm.autoConnect("ESP32-Arcade-Setup");

    // 連線完畢後重置螢幕刷頁
    initScreen();

    if (!res) {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.drawString("WiFi Connection Failed!", 10, 100, 4);
    } else {
        // 連線成功！畫出深綠色底面板
        tft.fillScreen(TFT_DARKGREEN);

        // 標題
        tft.setTextColor(TFT_YELLOW, TFT_DARKGREEN);
        tft.drawString("SYSTEM ONLINE", 10, 15, 4);

        // 畫一條像素風格分隔線
        tft.drawFastHLine(10, 50, 300, TFT_WHITE);

        // 顯示 SSID
        tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        tft.drawString("SSID :", 10, 70, 2);
        tft.setTextColor(TFT_GREENYELLOW, TFT_DARKGREEN);
        tft.drawString(WiFi.SSID(), 80, 70, 2);

        // 顯示 IP
        tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        tft.drawString("IP   :", 10, 100, 2);
        tft.setTextColor(TFT_GREENYELLOW, TFT_DARKGREEN);
        tft.drawString(WiFi.localIP().toString(), 80, 100, 2);

        // 底部提示
        tft.setTextColor(TFT_ORANGE, TFT_DARKGREEN);
        tft.drawString("Ready for next module...", 10, 180, 2);
    }
}

void loop() {
    // 輕量主迴圈，完全無負擔
}