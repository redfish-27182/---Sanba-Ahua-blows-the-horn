#pragma once
#include <TFT_eSPI.h>

class DisplayDriver {
public:
    // 建立螢幕操作介面，不取得 TFT 的所有權。
    // screen：TFT_eSPI&，已初始化的螢幕；
    explicit DisplayDriver(TFT_eSPI &screen) : tft(screen) {}
    
    // 清除螢幕並顯示狀態文字。
    // message：const char*，要顯示的訊息；
    void showMessage(const char *message);

    // 傳送一條 RGB565 畫面。
    // y、height：int，畫面位置及高度；pixels：uint16_t*，畫面寬度的資料；
    void drawStrip(int y, int height, uint16_t *pixels);

    // 開始一個完整畫面傳輸，避免每條 strip 重複開關 SPI transaction。
    // 輸入：無。
    void beginFrame();

    // 結束完整畫面傳輸並釋放 TFT 的 SPI transaction。
    // 輸入：無。
    void endFrame();

private:
    TFT_eSPI &tft;
};
