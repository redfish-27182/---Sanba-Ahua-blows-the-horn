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
    if (dmaEnabled) {
        tft.pushImageDMA(0, y, SCREEN_WIDTH, height, pixels);
        return;
    }
    tft.pushImage(0, y, SCREEN_WIDTH, height, pixels);
}

// 初始化 TFT DMA，呼叫前必須先釋放與 SD 共用的 VSPI bus。
// 輸出：bool，初始化是否成功。
bool DisplayDriver::enableDma() {
    if (dmaEnabled) return true;

    if (!tft.DMA_Enabled && !tft.initDMA()) {
        Serial.println("[DisplayDriver] TFT DMA 初始化失敗");
        return false;
    }
    dmaEnabled = tft.DMA_Enabled;
    Serial.printf("[DisplayDriver] TFT DMA：%s\n", dmaEnabled ? "已啟用" : "未啟用");
    return dmaEnabled;
}

// 開始一幀條帶繪製，讓後續 pushImage 共用同一次 SPI transaction。
// 輸入：無。
void DisplayDriver::beginFrame() {
    tft.setSwapBytes(true);
    tft.startWrite();
}

// 結束一幀條帶繪製，釋放 TFT 使用中的 SPI transaction。
// 輸入：無。
void DisplayDriver::endFrame() {
    if (dmaEnabled) {
        tft.dmaWait();
    }
    tft.endWrite();
}
