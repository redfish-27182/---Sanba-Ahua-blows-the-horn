#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFiManager.h>

#define TFT_BL 21
TFT_eSPI tft = TFT_eSPI();

// 🎯 CYD 小黃板專屬的獨立觸控 SPI 腳位
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// 建立獨立的 SPI 頻道與觸控物件
SPIClass touchSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// 記錄上一次觸控狀態 (用於判斷放開瞬間)
bool lastTouched = false;

void initHardware() {
    // 1. 背光與螢幕初始化
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1); // 橫屏 320x240
    tft.fillScreen(TFT_BLACK);

    // 2. 初始化獨立觸控 SPI (SCLK, MISO, MOSI, CS)
    touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(touchSpi);
    ts.setRotation(1); // 旋轉方向與螢幕一致
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n🚀=== 啟動小黃板獨立 XPT2046 觸控測試 ===🚀");

    initHardware();

    // 繪製標題
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("CYD INDEPENDENT TOUCH", 10, 10, 4);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Touch the screen now...", 10, 50, 2);

    // 畫出初始藍色按鈕
    tft.fillRect(40, 100, 240, 80, TFT_BLUE);
    tft.drawRect(40, 100, 240, 80, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.drawString("PRESS ME!", 100, 130, 4);
}

void loop() {
    // 🎯 1. 偵測是否有觸控
    if (ts.touched()) {
        TS_Point p = ts.getPoint();

        // XPT2046 Raw 原始數值 (約 200 ~ 3700) 轉成 320x240 螢幕座標
        int x = map(p.x, 200, 3700, 0, 320);
        int y = map(p.y, 240, 3800, 0, 240);

        // 數值邊界限制，避免超出螢幕
        x = constrain(x, 0, 320);
        y = constrain(y, 0, 240);

        Serial.printf("👉 [TOUCHED] Raw X: %d, Raw Y: %d -> Screen X: %d, Screen Y: %d\n", p.x, p.y, x, y);

        // 判斷是否按在按鈕區域內 (X: 40~280, Y: 100~180)
        if (x >= 40 && x <= 280 && y >= 100 && y <= 180) {
            tft.fillRect(40, 100, 240, 80, TFT_RED); // 按中變紅
            tft.setTextColor(TFT_YELLOW, TFT_RED);
            tft.drawString("PRESSED!", 105, 130, 4);
        }

        lastTouched = true; // 標記為按壓中
        delay(30);
    } 
    else {
        // 🎯 2. 放開手指的瞬間：恢復按鈕原本樣式
        if (lastTouched) {
            tft.fillRect(40, 100, 240, 80, TFT_BLUE);
            tft.drawRect(40, 100, 240, 80, TFT_WHITE);
            tft.setTextColor(TFT_WHITE, TFT_BLUE);
            tft.drawString("PRESS ME!", 100, 130, 4);

            Serial.println("🖐️ 手指已離開螢幕！");
            lastTouched = false; // 重置狀態
        }
    }

    delay(10);
}