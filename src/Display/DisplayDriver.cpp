#include "Display/DisplayDriver.h"
#include "Config.h"
#include "Font/SystemText.h"

// 顯示簡單的載入狀態或錯誤。
// message：const char*，顯示文字。
void DisplayDriver::showMessage(const char *message) {
    tft.fillScreen(TFT_BLACK);
    SystemText_Draw(tft, message, message, 12, 20, tft.width() - 24, tft.height() - 20, TFT_WHITE);
}

// 同步送出 RGB565 區塊，呼叫結束後可重用緩衝區。
// y、height：int，畫面位置及高度；pixels：uint16_t*，像素資料。
void DisplayDriver::drawStrip(int y, int height, uint16_t *pixels) {
    tft.setSwapBytes(true);
    tft.pushImage(0, y, SCREEN_WIDTH, height, pixels);
}
