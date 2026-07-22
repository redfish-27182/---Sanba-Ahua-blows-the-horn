#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <TFT_eSPI.h> // 引入你的螢幕函式庫

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "michael";
const char* password = "0932749747";
const String update_url = "http://192.168.0.101:8080/firmware.bin"; // 電腦 Python 伺服器網址

// 🎯 核心秘密武器：這是在下載過程中，會被不斷自動呼叫的「進度條函式」
void ota_progress_callback(int current_bytes, int total_bytes) {
    // 算目前的百分比 (%)
    int percentage = (current_bytes * 100) / total_bytes;
    
    Serial.printf("➔ [OTA 進度] 下載中: %d%%\n", percentage);

    // 🎨 在螢幕上繪製像素風進度條
    // 清除舊的百分比文字區域 (填滿黑色避免字疊在一起)
    tft.fillRect(100, 140, 120, 30, TFT_BLACK); 
    
    // 刷出當前的百分比文字
    tft.setTextColor(TFT_YELLOW);
    tft.drawCentreString(String(percentage) + "%", 160, 140, 4);

    // 畫一個簡易的進度條外框
    tft.drawRect(40, 180, 240, 20, TFT_WHITE);
    // 根據百分比填滿進度條 (240 像素寬 * 百分比)
    int bar_width = (240 * percentage) / 100;
    tft.fillRect(40, 180, bar_width, 20, TFT_GREEN);
}

void setup() {
    Serial.begin(9600);
    delay(1000);
    Serial.println("Starting OTA Update...");
    // 💡 強行把背光腳位 (GPIO 21) 拉高通電！
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    // 1. 初始化螢幕，並刷出漂亮的像素提示畫面
    tft.init();
    tft.setRotation(1); // 橫屏 320x240
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("SYSTEM UPDATE", 160, 40, 4);
    tft.setTextColor(TFT_GREEN);
    tft.drawCentreString("Connecting WiFi...", 160, 100, 2);

    // 2. 連接 Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    // 連線成功，畫面提示變更
    tft.fillRect(0, 100, 320, 40, TFT_BLACK); // 清除舊字
    tft.setTextColor(TFT_CYAN);
    tft.drawCentreString("Downloading Firmware...", 160, 90, 2);

    // 3. 🎯 綁定進度條回呼函式（關鍵步驟！）
    // 這行會告訴 ESP32：「下載時只要一有進度，就立刻去執行上面的 ota_progress_callback 函式」
    httpUpdate.onProgress(ota_progress_callback);

    // 允許在更新完畢後自動重啟
    httpUpdate.rebootOnUpdate(true); 

    // 4. 開始執行 OTA 下載
    WiFiClient client;
    t_httpUpdate_return ret = httpUpdate.update(client, update_url);

    // 如果執行到這裡，代表更新失敗了
    if (ret == HTTP_UPDATE_FAILED) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawCentreString("UPDATE FAILED!", 160, 100, 4);
    }
}

void loop() {
    // 舊版本不需要執行 loop
}