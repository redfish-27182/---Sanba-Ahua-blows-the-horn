#ifndef IMAGE_DOWNLOADER_H
#define IMAGE_DOWNLOADER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <FS.h>
#include <SD.h>

class ImageDownloader {
private:
    TFT_eSPI &tft;

    // 繪製藍色 UI 進度條 (風格與 GitHubOTA 保持一致)
    void drawProgressBar(int progress) {
        tft.fillRect(40, 140, 240, 20, TFT_BLACK);
        tft.drawRect(38, 138, 244, 24, TFT_WHITE);
        
        int fillWidth = map(progress, 0, 100, 0, 240);
        tft.fillRect(40, 140, fillWidth, 20, TFT_BLUE); // 圖片下載使用藍色進度條
        
        tft.fillRect(100, 175, 120, 30, TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(String(progress) + "%", 160, 190, 4);
    }

public:
    ImageDownloader(TFT_eSPI &tftScreen) : tft(tftScreen) {}

    /**
     * 核心圖片下載函式
     * @param downloadUrl 圖片 HTTP 網址
     * @param sdPath 存入 SD 卡的路徑 (如 "/cat.bmp")
     * @param showUI 是否顯示全螢幕進度條 (預設為 true；小更新/單張道具圖可設為 false 在背景下載)
     */
    bool downloadToSD(String downloadUrl, String sdPath, bool showUI = true) {
        if (downloadUrl.length() == 0) return false;

        if (showUI) {
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.setTextDatum(TC_DATUM);
            tft.drawString("DOWNLOADING ASSETS...", 160, 60, 4);
            drawProgressBar(0);
        }

        HTTPClient http;
        http.begin(downloadUrl);
        int httpCode = http.GET();

        if (httpCode != HTTP_CODE_OK) {
            Serial.printf("❌ 圖片下載失敗，HTTP Code: %d\n", httpCode);
            http.end();
            return false;
        }

       // 1. 詢問伺服器：「這張圖片總共有多大？」(單位是 Byte，例如一張圖可能是 230,400 Bytes)
        int totalLen = http.getSize();

        // 2. 在 SD 卡開啟/建立檔案（以「寫入模式 FILE_WRITE」開啟，準備把下載的資料放進去）
        File file = SD.open(sdPath, FILE_WRITE);

        // 3. 安全檢查：萬一 SD 卡沒插好、或是卡片滿了導致「無法建立檔案」
        if (!file) {
            Serial.println("❌ 無法建立 SD 卡檔案");
            http.end();      // 結束 HTTP 請求，釋房網路資源
            return false;    // 回傳失敗，中斷下載
        }

        // 4. 拿網路資料的「水龍頭」( Stream 指標 )
        //    伺服器傳過來的圖片資料會像水流一樣從這裡流出來
        WiFiClient *stream = http.getStreamPtr();

        // 5. 準備一個 512 Bytes 的小水桶 (緩衝區)
        //    因為 ESP32 記憶體太小，裝不下整張圖片，所以我們每次只裝 512 Bytes
        uint8_t buff[512] = {0};

        // 6. 記錄「目前已經下載了多少 Bytes」，一開始是 0
        int currentLen = 0;

        // 7. 核心下載迴圈：只要網路還連著，且檔案還沒傳完，就一直重複做裡面的事
        while (http.connected() && (totalLen > 0 || totalLen == -1)) {

            // 檢查「現在網路上有沒有剛傳到的二進位圖片資料流出來？」
            size_t size = stream->available();

            // 如果網路上有收到新資料（size 大於 0）
            if (size) {

                // 🎯 核心動作 1：用小水桶 (buff) 去接資料
                // min/三元運算子的意思是：「如果網路傳過來的資料大於 512，我們一次最多只拿 512；如果小於 512，有少就拿多少」
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                // 🎯 核心動作 2：把小水桶剛裝到的這 c 位元組資料，立刻「倒入 (寫入)」SD 卡中！
                file.write(buff, c);

                // 🎯 核心動作 3：累加已經下載的總量 (舊的下載量 + 剛才下載的 c)
                currentLen += c;

                // 🎯 核心動作 4：計算百分比並更新螢幕上的進度條
                if (showUI && totalLen > 0) {
                    // 公式：(目前下載量 * 100) / 圖片總大小 = 0 ~ 100 的百分比數字
                    int percent = (currentLen * 100) / totalLen;
                    
                    // 把算好的百分比傳給繪圖函式，畫出螢幕上的藍色進度條
                    drawProgressBar(percent);
                }
            }
            
            // 讓 ESP32 休息 1 毫秒（極度重要！防止晶片因為一直在跑迴圈而觸發看門狗 Watchdog 重啟）
            delay(1);
        }

        file.close();
        http.end();
        Serial.printf("🎉 圖片儲存成功: %s (%d bytes)\n", sdPath.c_str(), currentLen);
        return true;
    }
};

#endif