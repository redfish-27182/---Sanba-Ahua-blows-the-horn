#include "UI/Hud/HudCanvas.h"

#include <algorithm>

#include "Font/SystemText.h"

// 填滿裁切到目前條帶後的矩形。
// x、y、width、height：int，矩形；color：uint16_t，RGB565 色彩。
void HudCanvas::fillRect(int x, int y, int width, int height, uint16_t color) {
    const int left = std::max(0, x);
    const int right = std::min(bufferWidth, x + width);
    const int top = std::max(stripY, y);
    const int bottom = std::min(stripY + stripHeight, y + height);
    if (left >= right || top >= bottom) return;

    for (int row = top; row < bottom; ++row) {
        std::fill_n(pixels + (row - stripY) * bufferWidth + left, right - left, color);
    }
}

// 繪製裁切到目前條帶後的矩形外框。
// x、y、width、height：int，矩形；color：uint16_t，RGB565 色彩。
void HudCanvas::drawRect(int x, int y, int width, int height, uint16_t color) {
    if (width < 1 || height < 1) return;
    fillRect(x, y, width, 1, color);
    fillRect(x, y + height - 1, width, 1, color);
    fillRect(x, y, 1, height, color);
    fillRect(x + width - 1, y, 1, height, color);
}

// 填滿裁切到目前條帶後的像素圓角矩形。
// x、y、width、height、radius：int，矩形及圓角；color：uint16_t，RGB565 色彩。
void HudCanvas::fillRoundedRect(int x, int y, int width, int height, int radius, uint16_t color) {
    if (radius < 1 || width <= radius * 2 || height <= radius * 2) {
        fillRect(x, y, width, height, color);
        return;
    }
    fillRect(x + radius, y, width - radius * 2, 1, color);
    fillRect(x + 1, y + 1, width - 2, 1, color);
    fillRect(x, y + radius, width, height - radius * 2, color);
    fillRect(x + 1, y + height - 2, width - 2, 1, color);
    fillRect(x + radius, y + height - 1, width - radius * 2, 1, color);
}

// 繪製裁切到目前條帶後的像素圓角外框。
// x、y、width、height、radius：int，矩形及圓角；color：uint16_t，RGB565 色彩。
void HudCanvas::drawRoundedRect(int x, int y, int width, int height, int radius, uint16_t color) {
    if (radius < 1 || width <= radius * 2 || height <= radius * 2) {
        drawRect(x, y, width, height, color);
        return;
    }
    fillRect(x + radius, y, width - radius * 2, 1, color);
    fillRect(x + 1, y + 1, 1, 1, color);
    fillRect(x + width - 2, y + 1, 1, 1, color);
    fillRect(x, y + radius, 1, height - radius * 2, color);
    fillRect(x + width - 1, y + radius, 1, height - radius * 2, color);
    fillRect(x + 1, y + height - 2, 1, 1, color);
    fillRect(x + width - 2, y + height - 2, 1, 1, color);
    fillRect(x + radius, y + height - 1, width - radius * 2, 1, color);
}

// 將系統文字合成到目前條帶。
// zh、en：const char*，中文及英文備用文字；其餘為顯示範圍與樣式。
void HudCanvas::drawText(
    const char *zh,
    const char *en,
    int x,
    int y,
    int width,
    int height,
    uint16_t color,
    int letterSpacing
) {
    SystemText_DrawToBuffer(
        pixels,
        bufferWidth,
        stripY,
        stripHeight,
        zh,
        en,
        x,
        y,
        width,
        height,
        color,
        false,
        letterSpacing
    );
}
