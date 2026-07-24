#ifndef GITHUB_OTA_H
#define GITHUB_OTA_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <TFT_eSPI.h>

class GitHubOTA {
private:
    TFT_eSPI &tft;

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
    GitHubOTA(TFT_eSPI &tftScreen) : tft(tftScreen) {}

    bool startOTA(String downloadUrl) {
        Serial.printf("🔍 [DEBUG] 傳入 GitHubOTA 的實際網址: [%s]\n", downloadUrl.c_str());
        Serial.printf("🔍 [DEBUG] 網址字串長度: %d\n", downloadUrl.length());
        
        if (downloadUrl.length() == 0) {
            Serial.println("❌ 無效的下載網址");
            return false;
        }

        Serial.println("🚀 啟動 GitHub HTTPS OTA 升級程序...");

        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("OTA UPDATING...", 160, 60, 4);
        drawProgressBar(0);

        WiFiClientSecure client;
        client.setInsecure(); // 忽略 SSL 憑證驗證

        // 🎯 核心重點：維持你原本寫的 HTTPC_STRICT_FOLLOW_REDIRECTS 即可！
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        
        httpUpdate.onProgress([this](int current, int total) {
            int percent = (current * 100) / total;
            this->drawProgressBar(percent);
        });
        
        httpUpdate.rebootOnUpdate(true);

        // 直接交給 httpUpdate 自動處理 GitHub 302 重定向
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