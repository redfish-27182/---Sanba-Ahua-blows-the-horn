#include "Display/DisplayDriver.h"

// 顯示簡單的載入狀態或錯誤。
// message：const char*，顯示文字；無回傳值。
void DisplayDriver::showMessage(const char *message) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(message, 12, 20, 2);
}

// 同步送出 RGB565 區塊，呼叫結束後可重用緩衝區。
// y、height：int，畫面位置及高度；pixels：uint16_t*，像素資料；無回傳值。
void DisplayDriver::drawStrip(int y, int height, uint16_t *pixels) {
    tft.setSwapBytes(true);
    tft.pushImage(0, y, 320, height, pixels);
}
