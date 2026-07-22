#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <TFT_eSPI.h>

// 初始化 TFT 螢幕物件
TFT_eSPI tft = TFT_eSPI();

// ==================== 🛠️ 請在此處修改連線設定 ====================
const char* ssid = "michael"; 
const char* password = "0932749747"; 
const String update_url = "https://github.com/redfish-27182/---Sanba-Ahua-blows-the-horn/releases/download/v0.1.0-ota-test/update_test.bin";
// ==================================================================

// 🎨 OTA 下載進度條回呼函式 (即時更新 320x240 TFT 螢幕畫面)
void ota_progress_callback(int current_bytes, int total_bytes) {
    if (total_bytes <= 0) return;

    int percentage = (current_bytes * 100) / total_bytes;
    Serial.printf("➔ [GitHub OTA 進度] %d%%\n", percentage);

    // 1. 清除舊的百分比文字區域 (X:80, Y:130, W:160, H:40)
    tft.fillRect(80, 130, 160, 40, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW);
    tft.drawCentreString(String(percentage) + "%", 160, 135, 4);

    // 2. 繪製進度條外框與填充
    tft.drawRect(38, 178, 244, 24, TFT_WHITE); // 框線外框
    int bar_width = (240 * percentage) / 100;
    tft.fillRect(40, 180, bar_width, 20, TFT_GREEN); // 綠色進度條填滿
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 💡 點亮小黃板螢幕背光 (GPIO 21)
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);

    // 螢幕初始化與方向設定
    tft.init();
    tft.setRotation(1); // 橫屏 320x240
    tft.fillScreen(TFT_BLACK);

    // 顯示系統主標題
    tft.setTextColor(TFT_CYAN);
    tft.drawCentreString("SANBA AHUA OTA SYSTEM", 160, 20, 4);
    
    // 1. 開始連接 Wi-Fi
    Serial.println("正在連線至 Wi-Fi...");
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Connecting to WiFi...", 160, 70, 2);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\n✨ Wi-Fi 連線成功！");
    tft.fillRect(0, 65, 320, 30, TFT_BLACK); // 清除舊訊息
    tft.setTextColor(TFT_GREEN);
    tft.drawCentreString("WiFi Connected!", 160, 70, 2);

    // 2. 準備發起 GitHub HTTPS OTA 下載
    tft.setTextColor(TFT_ORANGE);
    tft.drawCentreString("Fetching update from GitHub...", 160, 105, 2);

    // 使用 WiFiClientSecure 處理 GitHub 的 HTTPS 密碼學協定
    WiFiClientSecure client;
    client.setInsecure(); // 跳過 SSL 憑證驗證 (嵌入式系統標準省資源做法)

    // 🎯 關鍵設定：允許 HTTPUpdate 自動跟隨 GitHub 的 302 重轉向 (Redirect)
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.onProgress(ota_progress_callback);
    httpUpdate.rebootOnUpdate(true); // 下載並刷寫完畢後自動重啟

    Serial.println("🚀 發起 GitHub HTTPUpdate 請求...");
    t_httpUpdate_return ret = httpUpdate.update(client, update_url);

    // 若程式繼續往下執行，代表 OTA 過程出錯
    if (ret == HTTP_UPDATE_FAILED) {
        int err_code = httpUpdate.getLastError();
        String err_str = httpUpdate.getLastErrorString();

        Serial.printf("❌ GitHub OTA 升級失敗！代碼 (%d): %s\n", err_code, err_str.c_str());

        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawCentreString("UPDATE FAILED!", 160, 80, 4);
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString("Error Code: " + String(err_code), 160, 130, 2);
        tft.drawCentreString(err_str, 160, 160, 2);
    }
}

void loop() {
    // OTA 測試階段 loop 無需執行任務
}