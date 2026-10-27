#pragma once

#include <Arduino.h>

// 可疊加到條帶緩衝區的螢幕座標圖層介面。
class ScreenOverlay {
public:
    virtual ~ScreenOverlay() = default;

    // 在送往螢幕前，把目前條帶內的像素疊加到 RGB565 緩衝區。
    // pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶的螢幕位置及高度。
    virtual void composeStrip(uint16_t *pixels, int screenY, int height) const = 0;
};
