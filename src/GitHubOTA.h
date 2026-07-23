#ifndef GITHUB_OTA_H
#define GITHUB_OTA_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <TFT_eSPI.h>

class GitHubOTA {
private:
    TFT_eSPI &tft; // 🎯 使用引用 (&)，直接綁定外部傳進來的 TFT 螢幕

    // 繪製 UI 進度條
    void drawProgressBar(int progress) {
        tft.fillRect(40, 140, 240, 20, TFT_BLACK);
        tft.drawRect(38, 138, 244, 24, TFT_WHITE);
        
        int fillWidth = map(progress, 0, 100, 0, 240);
        tft.fillRect(40, 140, fillWidth, 20, TFT_GREEN);
        
        tft.fillRect(100, 175, 120, 30, TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(String(progress) + "%", 160, 190, 4);
    }

public:
    // 建構子：接收外部的 tft 螢幕變數 (使用 &)
    GitHubOTA(TFT_eSPI &tftScreen) : tft(tftScreen) {}

    // 執行 HTTPS OTA 升級
    bool startOTA(String downloadUrl) {
        if (downloadUrl.length() == 0) {
            Serial.println("❌ 無效的下載網址");
            return false;
        }

        Serial.println("🚀 啟動 GitHub HTTPS OTA 升級程序...");

        // 直接像平時一樣用 . 來呼叫 tft 的功能！
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("OTA UPDATING...", 160, 60, 4);
        drawProgressBar(0);

        WiFiClientSecure client;
        client.setInsecure(); // 忽略 SSL 憑證驗證

        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        
        // 綁定進度條
        httpUpdate.onProgress([this](int current, int total) {
            int percent = (current * 100) / total;
            this->drawProgressBar(percent);
        });
        
        httpUpdate.rebootOnUpdate(true);

        t_httpUpdate_return ret = httpUpdate.update(client, downloadUrl);

        if (ret == HTTP_UPDATE_FAILED) {
            Serial.printf("❌ OTA 升級失敗 (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString("OTA FAILED!", 160, 100, 4);
            return false;
        }

        return true;
    }
};

#endif