#pragma once

#include <TFT_eSPI.h>
#include "Font/BitmapFont.h"

// 獨立測試畫面；離開時釋放系統字庫。
class FontTestScene {
public:
    // 建立測試畫面，借用現有 TFT。
    // screen：TFT_eSPI&，已初始化的螢幕；
    explicit FontTestScene(TFT_eSPI &screen) : tft(screen) {}

    // 從 SD 卡載入系統字庫並顯示測試畫面。
    bool open();

    // 重新繪製測試畫面或載入失敗訊息。
    void draw();

    // 離開時釋放字庫。
    void close();

private:
    TFT_eSPI &tft;
    BitmapFont font;
    bool ready = false;
};
