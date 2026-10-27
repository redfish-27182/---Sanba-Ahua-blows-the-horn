#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <memory>

// 單色 16×16 字庫，讀入 SRAM 後不再依賴 SD 卡。
class BitmapFont {
public:
    // 讀取並檢查轉檔工具產生的字庫；失敗時保留既有字庫。
    bool load(const char *path);

    // 釋放已載入的字庫。
    void unload();

    // 以 UTF-8 字串繪製文字，字庫缺字時使用 □。
    // tft：TFT_eSPI&，螢幕；text：const char*，UTF-8 文字；x、y：int，起點；color：uint16_t，RGB565 色彩。
    void drawText(TFT_eSPI &tft, const char *text, int x, int y, uint16_t color) const;

    // 在指定範圍繪製 UTF-8，依字元換行並避免超出範圍。
    // x、y、width、height：int，顯示區域；center：bool，是否逐行置中。
    void drawTextBox(TFT_eSPI &tft, const char *text, int x, int y, int width, int height, uint16_t color, bool center = false) const;

    // 將文字合成到指定螢幕條帶的 RGB565 緩衝區。
    // pixels：uint16_t*，條帶資料；bufferWidth、stripY、stripHeight：int，條帶尺寸與位置；letterSpacing：int，字距調整像素。
    void drawTextBoxToBuffer(
        uint16_t *pixels,
        int bufferWidth,
        int stripY,
        int stripHeight,
        const char *text,
        int x,
        int y,
        int width,
        int height,
        uint16_t color,
        bool center = false,
        int letterSpacing = 0
    ) const;

private:
    // 在已載入的排序索引中查詢字形。
    const uint8_t *findGlyph(uint32_t codepoint) const; // 查詢字形，若無則回傳 nullptr。
    std::unique_ptr<uint8_t[]> records; // 字形索引，已排序
    uint32_t count = 0; // 字形數量
};
