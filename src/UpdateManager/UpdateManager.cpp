#include "UpdateManager.h"

UpdateManager::UpdateManager(String serverUrl, String currentFwVersion, TFT_eSPI &tftScreen)
    : _tft(tftScreen), _ota(tftScreen), _imgDownloader(tftScreen) {
    _serverUrl = serverUrl;
    _currentFwVersion = currentFwVersion;
}

// 向 FastAPI 請求 JSON 配置檔
bool UpdateManager::fetchSystemConfig(SystemConfig &outConfig) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ [UpdateManager] Wi-Fi 未連線");
        return false;
    }

    HTTPClient http;
    http.begin(_serverUrl);
    http.setTimeout(4000);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("❌ [UpdateManager] HTTP 請求失敗 Code: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.printf("❌ [UpdateManager] JSON 解析失敗: %s\n", error.c_str());
        return false;
    }

    // 解析 JSON
    outConfig.firmwareVersion = doc["firmware_version"].as<String>();
    outConfig.firmwareUrl     = doc["firmware_url"].as<String>();

    outConfig.imageUrls.clear();
    JsonArray arr = doc["images"].as<JsonArray>();
    for (JsonVariant url : arr) {
        outConfig.imageUrls.push_back(url.as<String>());
    }

    return true;
}

// 🎯 【階段一】：檢查版本號是否不同
bool UpdateManager::hasPendingUpdate(SystemConfig &outConfig) {
    if (!fetchSystemConfig(outConfig)) {
        Serial.println("⚠️ [UpdateManager] 無法獲取伺服器配置，跳過更新比對。");
        return false;
    }

    // 只要伺服器版本號與本地寫死的版本不同，即代表需要更新 (包含圖片與韌體)
    if (outConfig.firmwareVersion != _currentFwVersion) {
        Serial.printf("⚡ [UpdateManager] 發現新版本 %s (本地 %s)！\n", 
                      outConfig.firmwareVersion.c_str(), _currentFwVersion.c_str());
        return true; 
    }

    Serial.println("✅ [UpdateManager] 當前已是最新版本，無需更新。");
    return false;
}

// 🎯 【階段二】：先載圖片，全成功才跑 OTA
void UpdateManager::executePendingUpdate(const SystemConfig &config) {
    Serial.println("🚀 [UpdateManager] 開始執行授權更新作業...");

    int totalImages = config.imageUrls.size();
    int successCount = 0;

    // 1. 下載所有圖片資產
    if (totalImages > 0) {
        Serial.printf("⚡ 準備下載 %d 張圖片資產...\n", totalImages);

        for (int i = 0; i < totalImages; i++) {
            bool downloadOk = _imgDownloader.downloadToSD(config.imageUrls[i], i + 1, totalImages);

            if (downloadOk) {
                successCount++;
            } else {
                Serial.printf("❌ 第 %d 張圖片下載失敗，中斷本輪更新！\n", i + 1);
                
                // 螢幕顯示下載失敗提示
                _tft.fillScreen(TFT_BLACK);
                _tft.setTextColor(TFT_RED, TFT_BLACK);
                _tft.setTextDatum(MC_DATUM);
                _tft.drawString("Download Failed!", _tft.width() / 2, _tft.height() / 2 - 10, 2);
                _tft.setTextColor(TFT_WHITE, TFT_BLACK);
                _tft.drawString("OTA Update Aborted.", _tft.width() / 2, _tft.height() / 2 + 15, 2);
                delay(2000);
                break; // 只要有一張失敗就終止，不啟動 OTA
            }
        }
    }

    // 2. 只有在「所有圖片皆下載成功」的情況下，才觸發 OTA 燒錄
    if (successCount == totalImages) {
        Serial.println("🎉 所有圖片下載成功！開始發起 GitHub OTA 燒錄...");
        
        if (config.firmwareUrl.length() > 0) {
            _ota.startOTA(config.firmwareUrl);
        } else {
            Serial.println("⚠️ 韌體網址空白，跳過 OTA。");
        }
    } else {
        Serial.println("⚠️ 圖片未完整下載，保護機制啟動，取消 OTA 燒錄。");
    }
}