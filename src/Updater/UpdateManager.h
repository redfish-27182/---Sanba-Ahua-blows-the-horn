#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <vector> // 引入 vector 容器來動態儲存多張圖片網址

#include "GitHubOTA.h"
#include "ImageDownloader.h"



// FastAPI 回傳的版本與資源配置資料結構
struct SystemConfig {
    String firmwareVersion;        // 韌體最新版本號 (如 "1.0.1")
    String firmwareUrl;            // 韌體 GitHub 下載網址
    int assetVersion;              // 大圖片資產包最新版本號 (如 101)
    std::vector<String> imageUrls; // 本次版本包含的所有圖片下載 URL 清單
};

class UpdateManager {
private:
    String _serverUrl;             // FastAPI 配置 API 網址 (如 http://.../api/v1/config)
    String _currentFwVersion;      // 本地當前編譯的韌體版本號
    Preferences _prefs;            // Flash NVS 讀寫物件 (用於記憶 asset_version)

    GitHubOTA _ota;                // 韌體 OTA 升級執行物件
    ImageDownloader _imgDownloader;// 單張圖片/檔案下載執行物件

public:
    // 建構子：傳入 Server 網址、當前韌體版本號與 TFT 螢幕物件
    UpdateManager(String serverUrl, String currentFwVersion, TFT_eSPI &tftScreen);

    // 初始化：讀取 Flash 中的舊資產版本號
    void begin();

    // 🎯 核心大更新檢查 (開機或觸發升級時呼叫)
    // 會自動進行 1. 韌體比對(OTA) 與 2. 圖片清單迴圈下載
    void checkMajorUpdates();

    // 🎯 未來小更新 / 單張道具圖下載 (不影響全域資產版本號)
    bool downloadSingleItem(String imageUrl, String sdPath, bool inBackground = true);

    // Flash NVS 版本號讀取與提交
    int getLocalAssetVersion();
    void commitAssetVersion(int newVersion);

    // 向 FastAPI 請求並解析 JSON 配置
    bool fetchSystemConfig(SystemConfig &outConfig);

    void resetAssetVersion();
};

#endif