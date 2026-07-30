#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <vector>

#include "GitHubOTA.h"
#include "ImageDownloader.h" // 直接引入同資料夾內的 Header-Only 模組

// FastAPI 回傳的版本與資源配置資料結構
struct SystemConfig {
    String firmwareVersion;        // 韌體與資產綁定的版本號 (如 "1.0.1")
    String firmwareUrl;            // 韌體 GitHub 下載網址
    std::vector<String> imageUrls; // 本次版本包含的所有圖片下載 URL 清單
};

class UpdateManager {
private:
    String _serverUrl;             // FastAPI 配置 API 網址
    String _currentFwVersion;      // 本地編譯寫死的韌體版本號 (CURRENT_FW_VERSION)
    TFT_eSPI &_tft;

    GitHubOTA _ota;                // 韌體 OTA 升級執行物件
    ImageDownloader _imgDownloader;// 單張圖片下載物件

public:
    UpdateManager(String serverUrl, String currentFwVersion, TFT_eSPI &tftScreen);

    // -------------------------------------------------------------------
    // 🎯 【階段一】：檢查是否有新版本 (只請求 JSON 與比對，不下載)
    // -------------------------------------------------------------------
    bool hasPendingUpdate(SystemConfig &outConfig);

    // -------------------------------------------------------------------
    // 🎯 【階段二】：使用者點選 [A] 確認後，先下載圖片，全數成功才發起 OTA
    // -------------------------------------------------------------------
    void executePendingUpdate(const SystemConfig &config);

    // 向 FastAPI 請求並解析 JSON 配置
    bool fetchSystemConfig(SystemConfig &outConfig);
};

#endif // UPDATE_MANAGER_H