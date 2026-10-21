#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <memory>

// 單色 16×16 字庫，讀入 SRAM 後不再依賴 SD 卡。
class BitmapFont {
public:
    // 讀取並檢查轉檔工具產生的字庫；失敗時保留既有字庫。
    // path：const char*，SD 卡字庫路徑；回傳：bool，是否成功。
    bool load(const char *path);

    // 釋放已載入的字庫。
    void unload();

    // 以 UTF-8 字串繪製文字，字庫缺字時使用 □。
    // tft：TFT_eSPI&，螢幕；text：const char*，UTF-8 文字；x、y：int，起點；color：uint16_t，RGB565 色彩。
    void drawText(TFT_eSPI &tft, const char *text, int x, int y, uint16_t color) const;

private:
    // 在已載入的排序索引中查詢字形。
    // codepoint：uint32_t，Unicode 編號；回傳：const uint8_t*，32 bytes 點陣或 nullptr。
    const uint8_t *findGlyph(uint32_t codepoint) const;
    std::unique_ptr<uint8_t[]> records;
    uint32_t count = 0;
};
