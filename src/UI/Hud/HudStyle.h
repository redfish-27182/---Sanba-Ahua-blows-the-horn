#pragma once

#include <Arduino.h>

// HUD 共用的明亮風格色彩與版面常數。
namespace HudStyle {
constexpr uint16_t SHADOW = 0x7DDF;
constexpr uint16_t PANEL = 0xFFFE;
constexpr uint16_t BORDER = 0x4E7F;
constexpr uint16_t TEXT = 0x18C3;
constexpr uint16_t BUTTON = 0xDFFF;
constexpr uint16_t SELECTED = 0xB7F5;

constexpr int MARGIN = 8;
constexpr int GLYPH_WIDTH = 16;
}
