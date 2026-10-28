#pragma once

#include <Arduino.h>

// 封裝條帶內的 HUD 像素繪製，不直接操作 TFT。
class HudCanvas {
public:
    // 建立目前條帶的繪圖工具。
    // pixels：uint16_t*，RGB565 條帶；bufferWidth、stripY、stripHeight：int，條帶資訊。
    HudCanvas(uint16_t *pixels, int bufferWidth, int stripY, int stripHeight)
        : pixels(pixels), bufferWidth(bufferWidth), stripY(stripY), stripHeight(stripHeight) {}

    // 在目前條帶填滿矩形。
    // x、y、width、height：int，矩形；color：uint16_t，RGB565 色彩。
    void fillRect(int x, int y, int width, int height, uint16_t color);

    // 在目前條帶繪製矩形外框。
    // x、y、width、height：int，矩形；color：uint16_t，RGB565 色彩。
    void drawRect(int x, int y, int width, int height, uint16_t color);

    // 在目前條帶填滿像素圓角矩形。
    // x、y、width、height、radius：int，矩形及圓角；color：uint16_t，RGB565 色彩。
    void fillRoundedRect(int x, int y, int width, int height, int radius, uint16_t color);

    // 在目前條帶繪製像素圓角外框。
    // x、y、width、height、radius：int，矩形及圓角；color：uint16_t，RGB565 色彩。
    void drawRoundedRect(int x, int y, int width, int height, int radius, uint16_t color);

    // 將系統文字寫入目前條帶。
    // zh、en：const char*，中文及英文備用文字；其餘為顯示範圍與樣式。
    void drawText(
        const char *zh,
        const char *en,
        int x,
        int y,
        int width,
        int height,
        uint16_t color,
        int letterSpacing = 0
    );

private:
    uint16_t *pixels;
    int bufferWidth;
    int stripY;
    int stripHeight;
};
