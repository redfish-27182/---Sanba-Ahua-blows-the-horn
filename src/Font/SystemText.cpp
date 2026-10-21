#include "Font/SystemText.h"
#include "Font/BitmapFont.h"

namespace {
    BitmapFont systemFont;
    bool ready = false;
}

// 從 SD 卡載入共用字庫。
bool SystemText_Load() {
    ready = systemFont.load("/fonts/system.bin");
    return ready;
}

// 選擇與目前字庫狀態相符的文字。
const char *SystemText_Choose(const char *zh, const char *en) { 
    return ready ? zh : en; 
}

// 字庫可用時繪製中文，否則使用內建英文字型。
// screen：螢幕；zh、en：文字；x、y、width、height：範圍；color：顏色；center：是否置中。
void SystemText_Draw(
    TFT_eSPI &screen, 
    const char *zh, 
    const char *en, 
    int x, 
    int y, 
    int width, 
    int height, 
    uint16_t color, 
    bool center
) { 
    // 字庫可用時使用 BitmapFont 繪製中文字串。
    if (ready) {
        systemFont.drawTextBox(screen, zh, x, y, width, height, color, center);
        return;
    }
    screen.setTextSize(1);
    screen.setTextColor(color);
    screen.setTextDatum(center ? TC_DATUM : TL_DATUM);

    // 字庫不可用時使用內建字型繪製英文備用文字。
    String line;
    while (*en && height >= 16) {
        char c = *en++;
        if (c == '\r') continue;
        if (c == '\n' || screen.textWidth(line + c, 2) > width) {
            screen.drawString(line, center ? x + width / 2 : x, y, 2);
            line = "";
            y += 16;
            height -= 16;
            if (height < 16) break;
            if (c == '\n') continue;
        }
        line += c;
    }

    // 最後一行文字尚未繪製，或是最後一行文字剛好換行但仍有空間。
    if (height >= 16 && line.length()) {
        screen.drawString(line, center ? x + width / 2 : x, y, 2);
    }
}
