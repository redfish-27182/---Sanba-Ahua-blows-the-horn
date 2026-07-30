#ifndef IMAGE_DOWNLOADER_H
#define IMAGE_DOWNLOADER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <TFT_eSPI.h>

class ImageDownloader {
private:
    TFT_eSPI &_tft;

    // 內部繪製下載狀態 (選項 A：簡潔文字風格)
    void showStatusUI(const String &fileName, int currentNum, int totalNum) {
        _tft.fillScreen(TFT_BLACK);
        
        // 標題
        _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        _tft.setTextDatum(TC_DATUM);
        _tft.drawString("[ Downloading Assets ]", _tft.width() / 2, 30, 2);

        // 進度資訊
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        _tft.setTextDatum(MC_DATUM);
        String progressStr = "Progress: " + String(currentNum) + " / " + String(totalNum);
        _tft.drawString(progressStr, _tft.width() / 2, _tft.height() / 2 - 10, 2);

        // 當前檔名
        _tft.setTextColor(TFT_GREEN, TFT_BLACK);
        _tft.drawString(fileName, _tft.width() / 2, _tft.height() / 2 + 20, 2);
    }

public:
    ImageDownloader(TFT_eSPI &tft) : _tft(tft) {}

    /**
     * @brief 下載單張圖片並存入 SD 卡根目錄
     * @param url 圖片 HTTP 網址
     * @param currentNum 當前第幾張
     * @param totalNum 總共幾張
     * @return true 下載成功，false 下載失敗
     */
    bool downloadToSD(const String &url, int currentNum, int totalNum) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("❌ [ImageDownloader] Wi-Fi 未連線");
            return false;
        }

        // 1. 自動從網址擷取檔名 (例如 "http://.../cat.bmp" -> "/cat.bmp")
        String fileName = url.substring(url.lastIndexOf('/'));
        if (!fileName.startsWith("/")) {
            fileName = "/" + fileName;
        }

        Serial.printf("📥 [%d/%d] 開始下載: %s\n", currentNum, totalNum, fileName.c_str());

        // 2. 顯示選項 A 風格 UI
        showStatusUI(fileName, currentNum, totalNum);

        // 3. 發起 HTTP GET 請求
        HTTPClient http;
        http.begin(url);
        http.setTimeout(5000);

        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
            Serial.printf("❌ [ImageDownloader] HTTP 請求失敗 Code: %d\n", httpCode);
            http.end();
            return false;
        }

        // 4. 開啟 SD 卡檔案 (同名檔案直接覆蓋)
        File file = SD.open(fileName, FILE_WRITE);
        if (!file) {
            Serial.println("❌ [ImageDownloader] 無法開啟 SD 卡檔案寫入！");
            http.end();
            return false;
        }

        // 5. 串流分段寫入 SD 卡
        WiFiClient *stream = http.getStreamPtr();
        uint8_t buffer[1024];
        int len = http.getSize();
        int totalDownloaded = 0;
        bool isSuccess = true;

        while (http.connected() && (len > 0 || len == -1)) {
            size_t sizeAvailable = stream->available();
            if (sizeAvailable) {
                int readBytes = stream->readBytes(buffer, ((sizeAvailable > sizeof(buffer)) ? sizeof(buffer) : sizeAvailable));
                file.write(buffer, readBytes);

                if (len > 0) {
                    len -= readBytes;
                }
                totalDownloaded += readBytes;
            }
            delay(1);
        }

        file.close();
        http.end();

        // 6. 失敗安全防護：若下載異常，刪除殘缺檔案
        if (len > 0) {
            Serial.printf("❌ [ImageDownloader] 檔案未下載完整，移除殘缺檔案: %s\n", fileName.c_str());
            SD.remove(fileName);
            isSuccess = false;
        } else {
            Serial.printf("✅ [ImageDownloader] 下載完成 (%d bytes): %s\n", totalDownloaded, fileName.c_str());
        }

        return isSuccess;
    }
};

#endif // IMAGE_DOWNLOADER_H