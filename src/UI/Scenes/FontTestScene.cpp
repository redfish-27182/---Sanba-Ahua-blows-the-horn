#include "UI/Scenes/FontTestScene.h"

// 載入測試字庫並顯示畫面。
// 無輸入；回傳：bool，字庫是否載入成功。
bool FontTestScene::open() {
    ready = font.load("/fonts/system.bin");
    draw();
    return ready;
}

// 顯示中英文與未收錄字元，確認字庫與缺字方框。
void FontTestScene::draw() {
    tft.fillScreen(TFT_BLACK);
    if (!ready) {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("Font load failed", 12, 20, 2);
        tft.drawString("Check /fonts/system.bin", 12, 45, 2);
        tft.drawString("Tap to return", 12, 70, 2);
        return;
    }
    // 「漢」不在初版系統清單，正常情況會顯示成 □。
    font.drawText(tft, "遊戲開始\n系統更新 ABC 123\n漢 □\n\n返回", 12, 20, TFT_WHITE);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Tap to return", 12, 200, 2);
}

// 退出測試並釋放字庫。
// 無輸入及回傳值。
void FontTestScene::close() { font.unload(); ready = false; }
