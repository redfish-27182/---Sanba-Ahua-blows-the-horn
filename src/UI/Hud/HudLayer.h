#pragma once

#include "Display/ScreenOverlay.h"
#include "EventTypes.h"

// 遊戲畫面上的固定 HUD，負責資訊面板、選單與其觸控攔截。
class HudLayer : public ScreenOverlay {
public:
    // 依觸控事件開關選單或選取項目。
    // event：const InputEvent&，觸控事件；回傳：bool，是否由 HUD 攔截。
    bool handleInput(const InputEvent &event);

    // 在送往螢幕前合成天氣面板、選單按鈕與已展開的選單。
    // pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶的螢幕位置及高度。
    void composeStrip(uint16_t *pixels, int screenY, int height) const override;

    // 取得並清除 HUD 要求的重繪標記。
    // 無輸入；回傳：bool，是否需要重繪。
    bool takeRedrawRequest();

private:
    enum class TouchCapture : uint8_t {
        NONE,
        WEATHER_PANEL,
        MENU_OVERLAY,
    };

    static constexpr int MARGIN = 8;
    static constexpr int WEATHER_WIDTH = 136;
    static constexpr int WEATHER_HEIGHT = 62;
    static constexpr int MENU_BUTTON_SIZE = 36;
    static constexpr int MENU_WIDTH = 112;
    static constexpr int MENU_ITEM_HEIGHT = 28;
    static constexpr int MENU_ITEM_COUNT = 3;

    bool menuOpen = false;
    bool redrawRequested = false;
    int selectedItem = -1;
    TouchCapture capture = TouchCapture::NONE;

    // 判斷座標是否位於右上選單按鈕。
    // x、y：int，螢幕座標；回傳：bool，是否命中。
    bool isMenuButton(int x, int y) const;

    // 取得下拉選單中被點選的項目索引。
    // x、y：int，螢幕座標；回傳：int，0 到 2 或 -1。
    int menuItemAt(int x, int y) const;

    // 繪製左上角的暫定天氣資訊到指定條帶。
    // pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶資訊。
    void drawWeatherPanel(uint16_t *pixels, int screenY, int height) const;

    // 繪製右上角三橫線選單按鈕到指定條帶。
    // pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶資訊。
    void drawMenuButton(uint16_t *pixels, int screenY, int height) const;

    // 繪製從選單按鈕下方展開的選項到指定條帶。
    // pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶資訊。
    void drawMenu(uint16_t *pixels, int screenY, int height) const;
};
