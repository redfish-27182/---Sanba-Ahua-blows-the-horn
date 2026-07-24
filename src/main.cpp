#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFiManager.h>
#include <SPI.h>
#include <SD.h>

#include "Updater/UpdateManager.h"// 引入我們的 Updater 獨立模組包

#define SD_CS 5

TFT_eSPI tft = TFT_eSPI();

// 設定 FastAPI 網址與當前編譯的韌體版本號
const String SERVER_URL     = "http://192.168.0.101:8000/api/v1/config";
const String CURRENT_FW_VER = "1.0.0";

// 實體化 UpdateManager，將 tft 螢幕物件傳進去
UpdateManager updater(SERVER_URL, CURRENT_FW_VER, tft);

void setup() {
    Serial.begin(115200);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // 💡 開啟背光

    tft.init();
    tft.setRotation(1);

    //updater.resetAssetVersion();

    // 初始化 SD 卡
    SPIClass sdSpi = SPIClass(HSPI);
    sdSpi.begin(18, 19, 23, SD_CS);
    SD.begin(SD_CS, sdSpi);

    WiFiManager wm;
    wm.autoConnect("黃色小板板的測試");

    updater.begin();

    // 🚀 執行大更新 (比對程式與大圖片資產包版本)
    updater.checkMajorUpdates();

   
    Serial.println("進入主程序...");
}

void loop() {
    // 處理常態 UI
}