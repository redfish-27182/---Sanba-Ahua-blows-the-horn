#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// ---------------- HARDWARE & CONSTANTS ----------------
#define TFT_BL 21               // 小黃板 LCD 背光腳位
const char* CURRENT_VERSION = "v0.0.0"; // ESP32 目前的版本 (更改此版本號測試 API 邏輯)
const char* ssid = "michael";
const char* password = "0932749747";

// FastAPI 伺服器網址 (請換成你電腦的區域 IP)
// 範例：http://192.168.0.100:8000/check_update
const String server_api_url = "http://192.168.0.101:8000/check_update";

TFT_eSPI tft = TFT_eSPI();

// ---------------- UI & PROGRESS BAR ----------------
void draw_ota_ui(int progress) {
    tft.fillRect(40, 140, 240, 20, TFT_BLACK);
    tft.drawRect(38, 138, 244, 24, TFT_WHITE);
    
    int fillWidth = map(progress, 0, 100, 0, 240);
    tft.fillRect(40, 140, fillWidth, 20, TFT_GREEN);
    
    tft.fillRect(100, 175, 120, 30, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(progress) + "%", 160, 190, 4);
}

void ota_progress_callback(int current, int total) {
    static int last_percent = -1;
    int percent = (current * 100) / total;
    if (percent != last_percent) {
        last_percent = percent;
        Serial.printf("📥 OTA 下載進度: %d%%\n", percent);
        draw_ota_ui(percent);
    }
}

// ---------------- HTTPS OTA DOWNLOAD ----------------
void perform_ota(String download_url) {
    Serial.println("🚀 開始向 GitHub 請求 OTA 下載...");
    
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("OTA UPDATING...", 160, 60, 4);
    draw_ota_ui(0);

    WiFiClientSecure client;
    client.setInsecure(); // 忽略 GitHub 的 SSL 憑證驗證

    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // 跟隨 302 轉向
    httpUpdate.onProgress(ota_progress_callback);
    httpUpdate.rebootOnUpdate(true);

    t_httpUpdate_return ret = httpUpdate.update(client, download_url);

    if (ret == HTTP_UPDATE_FAILED) {
        Serial.printf("❌ OTA 下載失敗 (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("OTA FAILED!", 160, 100, 4);
    }
}

// ---------------- HTTP GET CHECK UPDATE ----------------
void check_for_updates() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    // 組合 GET 網址帶上版本號參數: /check_update?current_version=v0.0.0
    String full_url = server_api_url + "?current_version=" + CURRENT_VERSION;
    
    Serial.print("🔍 正在向 FastAPI 查詢更新: ");
    Serial.println(full_url);

    http.begin(full_url);
    int httpCode = http.GET(); // 發送 GET 請求

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("📄 收到 FastAPI 回應:");
        Serial.println(payload);

        // 解析 JSON
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            bool has_update = doc["has_update"];
            const char* latest_version = doc["latest_version"];
            const char* download_url = doc["download_url"];
            const char* message = doc["message"];

            Serial.printf("📢 訊息: %s\n", message);

            if (has_update) {
                Serial.printf("✨ 發現新版本 %s！準備啟動 OTA...\n", latest_version);
                perform_ota(String(download_url));
            } else {
                Serial.println("✅ 目前為最新版，跳過 OTA，進入主程式。");
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                tft.drawString("SYSTEM UP TO DATE", 160, 120, 4);
            }
        } else {
            Serial.print("❌ JSON 解析失敗: ");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("❌ 查詢 API 失敗，HTTP 代碼: %d\n", httpCode);
    }

    http.end();
}

// ---------------- SETUP & LOOP ----------------
void setup() {
    Serial.begin(115200);

    // 初始化 TFT 螢幕與背光
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Connecting Wi-Fi...", 160, 120, 4);

    // 連接 Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ Wi-Fi 已連線！");
    tft.fillScreen(TFT_BLACK);

    // 🎯 關鍵步驟：發起 HTTP GET 檢查 API
    check_for_updates();
}

void loop() {
    // 主程式邏輯 (若未觸發 OTA 重啟，將在此處運行)
    delay(1000);
}