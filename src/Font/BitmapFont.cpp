#include "Font/BitmapFont.h"

#include <SD.h>
#include <cstring>
#include <new>
#include <utility>

namespace {
constexpr size_t headerSize = 16;
constexpr size_t recordSize = 36;
constexpr uint32_t fallbackCodepoint = 0x25a1;

// 讀取小端序 16 位元整數。
// data：const uint8_t*，至少兩個位元組；回傳：uint16_t，解碼結果。
uint16_t read16(const uint8_t *data) {
    return uint16_t(data[0]) | (uint16_t(data[1]) << 8);
}

// 讀取小端序 32 位元整數。
// data：const uint8_t*，至少四個位元組；回傳：uint32_t，解碼結果。
uint32_t read32(const uint8_t *data) {
    return uint32_t(read16(data)) | (uint32_t(read16(data + 2)) << 16);
}

// 讀取下一個 UTF-8 字元，格式錯誤時回傳缺字標記。
// cursor：const char*&，會前進的字串位置；回傳：uint32_t，Unicode 編號。
uint32_t nextCodepoint(const char *&cursor) {
    const uint8_t first = static_cast<uint8_t>(*cursor++);
    if (first < 0x80) return first;
    int extra = 0;
    uint32_t value = 0;
    uint32_t minimum = 0;
    if (first >= 0xc2 && first <= 0xdf) { extra = 1; value = first & 0x1f; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { extra = 2; value = first & 0x0f; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { extra = 3; value = first & 0x07; minimum = 0x10000; }
    else return fallbackCodepoint;
    const char *next = cursor;
    for (int i = 0; i < extra; ++i) {
        const uint8_t byte = static_cast<uint8_t>(*next);
        if (byte == 0 || (byte & 0xc0) != 0x80) return fallbackCodepoint;
        value = (value << 6) | (byte & 0x3f);
        ++next;
    }
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return fallbackCodepoint;
    cursor = next;
    return value;
}
}

// 載入固定格式的字庫，驗證大小、字元順序與缺字標記。
// path：const char*，SD 卡字庫路徑；回傳：bool，是否成功。
bool BitmapFont::load(const char *path) {
    File file = SD.open(path, FILE_READ);
    if (!file) { Serial.printf("[BitmapFont] cannot open: %s\n", path); return false; }
    uint8_t header[headerSize];
    if (file.read(header, sizeof(header)) != sizeof(header) || memcmp(header, "AHFT", 4) != 0 ||
        read16(header + 4) != 1 || read16(header + 6) != 16 || read16(header + 8) != 16 || read16(header + 10) != 0) {
        Serial.println("[BitmapFont] invalid header"); return false;
    }
    const uint32_t glyphCount = read32(header + 12);
    if (!glyphCount || glyphCount > 4096 || file.size() != headerSize + size_t(glyphCount) * recordSize) {
        Serial.println("[BitmapFont] invalid count or file size"); return false;
    }
    const size_t bytes = size_t(glyphCount) * recordSize;
    std::unique_ptr<uint8_t[]> loaded(new (std::nothrow) uint8_t[bytes]);
    if (!loaded) {
        Serial.printf("[BitmapFont] allocation failed: %u bytes, free=%u, largest=%u\n", static_cast<unsigned>(bytes), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        return false;
    }
    if (file.read(loaded.get(), bytes) != bytes) { Serial.println("[BitmapFont] incomplete file"); return false; }
    uint32_t previous = 0;
    bool hasFallback = false;
    for (uint32_t i = 0; i < glyphCount; ++i) {
        const uint32_t codepoint = read32(loaded.get() + size_t(i) * recordSize);
        if ((i && codepoint <= previous) || codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff)) {
            Serial.println("[BitmapFont] invalid character index"); return false;
        }
        if (codepoint == fallbackCodepoint) hasFallback = true;
        previous = codepoint;
    }
    if (!hasFallback) { Serial.println("[BitmapFont] missing fallback glyph"); return false; }
    records = std::move(loaded);
    count = glyphCount;
    Serial.printf("[BitmapFont] loaded %u characters, %u bytes; free=%u, largest=%u\n", count, static_cast<unsigned>(bytes), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    return true;
}

// 釋放字庫資料。
// 無輸入及回傳值。
void BitmapFont::unload() { records.reset(); count = 0; }

// 以二分搜尋查詢字形。
// codepoint：uint32_t，Unicode 編號；回傳：const uint8_t*，字形或 nullptr。
const uint8_t *BitmapFont::findGlyph(uint32_t codepoint) const {
    uint32_t low = 0, high = count;
    while (low < high) {
        const uint32_t mid = low + (high - low) / 2;
        const uint32_t value = read32(records.get() + size_t(mid) * recordSize);
        if (value < codepoint) low = mid + 1;
        else high = mid;
    }
    if (low == count || read32(records.get() + size_t(low) * recordSize) != codepoint) return nullptr;
    return records.get() + size_t(low) * recordSize + 4;
}

// 依點陣繪製連續的墨水區段，保留原本背景。
// tft：TFT_eSPI&，螢幕；text：const char*，UTF-8 文字；x、y：int，起點；color：uint16_t，顏色。
// 無回傳值。
void BitmapFont::drawText(TFT_eSPI &tft, const char *text, int x, int y, uint16_t color) const {
    if (!text || !records || x < 0 || y < 0 || x + 16 > tft.width()) return;
    const int startX = x;
    while (*text && y + 16 <= tft.height()) {
        const uint32_t codepoint = nextCodepoint(text);
        if (codepoint == '\r') continue;
        if (codepoint == '\n') { x = startX; y += 16; continue; }
        if (x + 16 > tft.width()) { x = startX; y += 16; }
        if (y + 16 > tft.height()) break;
        const uint8_t *glyph = findGlyph(codepoint);
        if (!glyph) glyph = findGlyph(fallbackCodepoint);
        if (glyph) {
            for (int row = 0; row < 16; ++row) {
                const uint16_t bits = (uint16_t(glyph[row * 2]) << 8) | glyph[row * 2 + 1];
                for (int column = 0; column < 16;) {
                    if (!(bits & (0x8000u >> column))) { ++column; continue; }
                    const int begin = column;
                    while (column < 16 && (bits & (0x8000u >> column))) ++column;
                    tft.drawFastHLine(x + begin, y + row, column - begin, color);
                }
            }
        }
        x += 16;
    }
}

// 將文字限制在矩形範圍，以完整 UTF-8 字元計算每一行。
// tft：TFT_eSPI&；text：UTF-8 字串；x、y、width、height：區域；color：顏色；center：是否置中。
void BitmapFont::drawTextBox(TFT_eSPI &tft, const char *text, int x, int y, int width, int height, uint16_t color, bool center) const {
    if (!text || !records || x < 0 || y < 0 || width < 16 || height < 16) return;
    if (x + width > tft.width()) width = tft.width() - x;
    if (y + height > tft.height()) height = tft.height() - y;
    const int columns = width / 16;
    if (columns < 1) return;
    while (*text && height >= 16) {
        const char *lineStart = text;
        int countInLine = 0;
        while (*text && countInLine < columns) {
            const char *next = text;
            uint32_t codepoint = nextCodepoint(next);
            if (codepoint == '\n') break;
            text = next;
            if (codepoint != '\r') ++countInLine;
        }
        int px = x + (center ? (width - countInLine * 16) / 2 : 0);
        while (lineStart < text) {
            const uint32_t codepoint = nextCodepoint(lineStart);
            if (codepoint == '\r') continue;
            const uint8_t *glyph = findGlyph(codepoint);
            if (!glyph) glyph = findGlyph(fallbackCodepoint);
            if (glyph) for (int row = 0; row < 16; ++row) {
                uint16_t bits = (uint16_t(glyph[row * 2]) << 8) | glyph[row * 2 + 1];
                for (int col = 0; col < 16;) {
                    if (!(bits & (0x8000u >> col))) { ++col; continue; }
                    int begin = col;
                    while (col < 16 && (bits & (0x8000u >> col))) ++col;
                    tft.drawFastHLine(px + begin, y + row, col - begin, color);
                }
            }
            px += 16;
        }
        if (*text == '\n') ++text;
        y += 16;
        height -= 16;
    }
}
