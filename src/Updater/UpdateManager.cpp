#include "UpdateManager.h"

UpdateManager::UpdateManager(String serverUrl, String currentFwVersion, TFT_eSPI &tftScreen)
    : _ota(tftScreen), _imgDownloader(tftScreen) {
    _serverUrl = serverUrl;
    _currentFwVersion = currentFwVersion;
}

void UpdateManager::begin() {
    int localVer = getLocalAssetVersion();
    Serial.printf("📌 [UpdateManager] 初始化完成 | 當前韌體: %s | 本地圖片資產版本: v%d\n", _currentFwVersion.c_str(), localVer);
}

// 讀取 Flash NVS 中的舊版本號
int UpdateManager::getLocalAssetVersion() {
    _prefs.begin("sys_ver", true); // true = Read-Only
    int ver = _prefs.getInt("asset_ver", 0);
    _prefs.end();
    return ver;
}

// 將新的資產版本號 Commit 寫入 Flash NVS
void UpdateManager::commitAssetVersion(int newVersion) {
    _prefs.begin("sys_ver", false); // false = Read-Write
    _prefs.putInt("asset_ver", newVersion);
    _prefs.end();
    Serial.printf("💾 [UpdateManager] Flash 圖片資產版本成功更新至: v%d\n", newVersion);
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
        Serial.printf("❌ [UpdateManager] HTTP 請求失敗，Code: %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // 解析包含 images 陣列的 JSON 配置
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.printf("❌ [UpdateManager] JSON 解析失敗: %s\n", error.c_str());
        return false;
    }

    outConfig.firmwareVersion = doc["firmware_version"].as<String>();
    outConfig.firmwareUrl     = doc["firmware_url"].as<String>();
    outConfig.assetVersion    = doc["asset_version"].as<int>();

    // 🎯 讀取 images 圖片網址陣列
    outConfig.imageUrls.clear();
    JsonArray arr = doc["images"].as<JsonArray>();
    for (JsonVariant url : arr) {
        outConfig.imageUrls.push_back(url.as<String>());
    }

    return true;
}

// 🎯 核心大更新邏輯 (OTA + 多圖片迴圈下載)
void UpdateManager::checkMajorUpdates() {
    SystemConfig config;

    // 1. 向 FastAPI 獲取配置資訊
    if (!fetchSystemConfig(config)) {
        Serial.println("⚠️ 無法取得伺服器配置，跳過大更新比對。");
        return;
    }

    // 2. 韌體 OTA 比對與升級 (成功會自動重啟晶片)
    if (config.firmwareVersion != _currentFwVersion && config.firmwareUrl.length() > 0) {
        Serial.printf("⚡ 發現新韌體 %s (目前 %s)，發起 GitHub OTA...\n", config.firmwareVersion.c_str(), _currentFwVersion.c_str());
        _ota.startOTA(config.firmwareUrl);
    } else {
        Serial.println("✅ 韌體已是最新版本。");
    }

    // 3. 多張圖片資產包比對與迴圈下載
    int localVer = getLocalAssetVersion();
    if (config.assetVersion > localVer) {
        int totalImages = config.imageUrls.size();
        int successCount = 0;

        Serial.printf("⚡ 發現新圖片包 v%d (本地 v%d)，共 %d 張圖片待下載...\n", config.assetVersion, localVer, totalImages);

        // 用 for 迴圈將清單裡的網址「一張一張」抽出來傳給 ImageDownloader
        for (int i = 0; i < totalImages; i++) {
            String imgUrl = config.imageUrls[i];

            // 自動從網址最後面切出檔名 (例如 "http://.../v101/cat.bmp" 轉成 "/cat.bmp")
            String fileName = imgUrl.substring(imgUrl.lastIndexOf('/'));

            Serial.printf("📥 [%d/%d] 開始下載圖片: %s\n", i + 1, totalImages, fileName.c_str());

            // 呼叫黑盒子 ImageDownloader 進行單張下載 (顯示進度條 UI)
            bool downloadOk = _imgDownloader.downloadToSD(imgUrl, fileName, true);

            if (downloadOk) {
                successCount++;
            } else {
                Serial.printf("❌ 第 %d 張圖片 (%s) 下載失敗，中斷本輪更新。\n", i + 1, fileName.c_str());
                break; // 只要有一張失敗就中斷，保護版本號不被寫入
            }
        }

        // 🔒 安全保護機制：只有「全部圖片都下載成功」才改寫 Flash 版本號！
        if (successCount == totalImages && totalImages > 0) {
            commitAssetVersion(config.assetVersion);
            Serial.println("🎉 所有圖片資產均成功存入 SD 卡，並記錄最新版本號！");
        } else {
            Serial.println("⚠️ 圖片下載未全部完成，Flash 保持舊版本，下次將重新下載。");
        }
    } else {
        Serial.println("✅ 圖片資產已是最新，跳過下載！");
    }
}

// 🎯 未來小更新 / 單張道具圖下載 (背景/前台下載)
bool UpdateManager::downloadSingleItem(String imageUrl, String sdPath, bool inBackground) {
    Serial.printf("📥 [小更新] 下載單張圖片: %s -> %s\n", imageUrl.c_str(), sdPath.c_str());
    return _imgDownloader.downloadToSD(imageUrl, sdPath, !inBackground);
}

void UpdateManager::resetAssetVersion() {
    _prefs.begin("sys_ver", false);
    _prefs.clear(); // 🗑️ 擦除這個 Namespace 下的所有資料
    _prefs.end();
    Serial.println("🧹 已清空 Flash NVS 版本記錄！");
}