#include "UI/Hud/HudLayer.h"

#include <algorithm>

#include "Config.h"
#include "Font/SystemText.h"

namespace {
constexpr uint16_t HUD_SHADOW = 0x7DDF;
constexpr uint16_t HUD_PANEL = 0xFFFE;
constexpr uint16_t HUD_BORDER = 0x4E7F;
constexpr uint16_t HUD_TEXT = 0x18C3;
constexpr uint16_t HUD_BUTTON = 0xDFFF;
constexpr uint16_t HUD_SELECTED = 0xB7F5;

constexpr const char *MENU_LABELS_ZH[] = {"設定", "資訊", "說明"};
constexpr const char *MENU_LABELS_EN[] = {"SETTINGS", "INFO", "HELP"};

// 把矩形裁切到目前條帶後填入 RGB565 像素。
// pixels：uint16_t*，條帶；screenY、stripHeight：int，條帶資訊；其餘為矩形資料。
void fillRectToStrip(
    uint16_t *pixels,
    int screenY,
    int stripHeight,
    int x,
    int y,
    int width,
    int height,
    uint16_t color
) {
    const int left = std::max(0, x);
    const int right = std::min(SCREEN_WIDTH, x + width);
    const int top = std::max(screenY, y);
    const int bottom = std::min(screenY + stripHeight, y + height);
    if (left >= right || top >= bottom) return;

    for (int row = top; row < bottom; ++row) {
        std::fill_n(pixels + (row - screenY) * SCREEN_WIDTH + left, right - left, color);
    }
}

// 把矩形外框裁切到目前條帶後寫入 RGB565 像素。
// pixels：uint16_t*，條帶；screenY、stripHeight：int，條帶資訊；其餘為矩形資料。
void drawRectToStrip(
    uint16_t *pixels,
    int screenY,
    int stripHeight,
    int x,
    int y,
    int width,
    int height,
    uint16_t color
) {
    if (width < 1 || height < 1) return;
    fillRectToStrip(pixels, screenY, stripHeight, x, y, width, 1, color);
    fillRectToStrip(pixels, screenY, stripHeight, x, y + height - 1, width, 1, color);
    fillRectToStrip(pixels, screenY, stripHeight, x, y, 1, height, color);
    fillRectToStrip(pixels, screenY, stripHeight, x + width - 1, y, 1, height, color);
}

// 將系統文字寫入目前條帶。
// pixels：uint16_t*，條帶；screenY、stripHeight：int，條帶資訊；其餘為文字資料。
void drawTextToStrip(
    uint16_t *pixels,
    int screenY,
    int stripHeight,
    const char *zh,
    const char *en,
    int x,
    int y,
    int width,
    int height,
    uint16_t color
) {
    SystemText_DrawToBuffer(
        pixels,
        SCREEN_WIDTH,
        screenY,
        stripHeight,
        zh,
        en,
        x,
        y,
        width,
        height,
        color
    );
}
}

// 依觸控事件處理 HUD，並在需要時要求遊戲畫面重繪。
// event：const InputEvent&，觸控事件；回傳：bool，是否由 HUD 攔截。
bool HudLayer::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TOUCH_DOWN) {
        // 展開選單後先攔截全部觸控，避免點選空白處時拖動地圖。
        if (menuOpen) {
            capture = TouchCapture::MENU_OVERLAY;
            return true;
        }

        // 天氣面板暫不提供操作，但仍攔截從面板開始的手勢。
        if (event.x >= MARGIN && event.x < MARGIN + WEATHER_WIDTH
            && event.y >= MARGIN && event.y < MARGIN + WEATHER_HEIGHT) {
            capture = TouchCapture::WEATHER_PANEL;
            return true;
        }

        if (isMenuButton(event.x, event.y)) {
            capture = TouchCapture::MENU_OVERLAY;
            return true;
        }

        return false;
    }

    // 未從 HUD 開始的觸控維持交由地圖處理。
    if (capture == TouchCapture::NONE) {
        return false;
    }

    if (event.type == InputEventType::TAP) {
        if (capture == TouchCapture::MENU_OVERLAY) {
            if (isMenuButton(event.x, event.y)) {
                menuOpen = !menuOpen;
                selectedItem = -1;
                redrawRequested = true;
            } else if (menuOpen) {
                const int item = menuItemAt(event.x, event.y);
                if (item >= 0) {
                    selectedItem = item;
                    redrawRequested = true;
                } else {
                    menuOpen = false;
                    selectedItem = -1;
                    redrawRequested = true;
                }
            }
        }
        capture = TouchCapture::NONE;
    } else if (event.type == InputEventType::RELEASE
        || event.type == InputEventType::LONG_PRESS
        || event.type == InputEventType::SWIPE) {
        capture = TouchCapture::NONE;
    }

    return true;
}

// 在每一條地圖條帶內合成 HUD，避免直接繪圖造成閃爍。
// pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶的螢幕位置及高度。
void HudLayer::composeStrip(uint16_t *pixels, int screenY, int height) const {
    drawWeatherPanel(pixels, screenY, height);
    drawMenuButton(pixels, screenY, height);
    if (menuOpen) {
        drawMenu(pixels, screenY, height);
    }
}

// 取得並清除 HUD 重繪要求。
// 無輸入；回傳：bool，是否需要重繪。
bool HudLayer::takeRedrawRequest() {
    const bool requested = redrawRequested;
    redrawRequested = false;
    return requested;
}

// 判斷座標是否命中右上角的選單按鈕。
// x、y：int，螢幕座標；回傳：bool，是否命中。
bool HudLayer::isMenuButton(int x, int y) const {
    const int buttonX = SCREEN_WIDTH - MARGIN - MENU_BUTTON_SIZE;
    return x >= buttonX && x < buttonX + MENU_BUTTON_SIZE
        && y >= MARGIN && y < MARGIN + MENU_BUTTON_SIZE;
}

// 取得選單項目的索引。
// x、y：int，螢幕座標；回傳：int，0 到 2 或 -1。
int HudLayer::menuItemAt(int x, int y) const {
    const int menuX = SCREEN_WIDTH - MARGIN - MENU_WIDTH;
    const int menuY = MARGIN + MENU_BUTTON_SIZE + 4;
    const int menuHeight = MENU_ITEM_HEIGHT * MENU_ITEM_COUNT;
    if (x < menuX || x >= menuX + MENU_WIDTH || y < menuY || y >= menuY + menuHeight) {
        return -1;
    }
    return (y - menuY) / MENU_ITEM_HEIGHT;
}

// 繪製左上角的暫定天氣資料到目前條帶。
// pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶資訊。
void HudLayer::drawWeatherPanel(uint16_t *pixels, int screenY, int height) const {
    fillRectToStrip(pixels, screenY, height, MARGIN + 2, MARGIN + 2, WEATHER_WIDTH, WEATHER_HEIGHT, HUD_SHADOW);
    fillRectToStrip(pixels, screenY, height, MARGIN, MARGIN, WEATHER_WIDTH, WEATHER_HEIGHT, HUD_PANEL);
    drawRectToStrip(pixels, screenY, height, MARGIN, MARGIN, WEATHER_WIDTH, WEATHER_HEIGHT, HUD_BORDER);
    drawTextToStrip(pixels, screenY, height, "天氣：--", "Weather: --", MARGIN + 8, MARGIN + 5, WEATHER_WIDTH - 16, 16, HUD_TEXT);
    drawTextToStrip(pixels, screenY, height, "溫度：--°C", "Temp: --C", MARGIN + 8, MARGIN + 23, WEATHER_WIDTH - 16, 16, HUD_TEXT);
    drawTextToStrip(pixels, screenY, height, "濕度：--%", "Humidity: --%", MARGIN + 8, MARGIN + 41, WEATHER_WIDTH - 16, 16, HUD_TEXT);
}

// 繪製右上角的三橫線選單按鈕到目前條帶。
// pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶資訊。
void HudLayer::drawMenuButton(uint16_t *pixels, int screenY, int height) const {
    const int buttonX = SCREEN_WIDTH - MARGIN - MENU_BUTTON_SIZE;
    fillRectToStrip(pixels, screenY, height, buttonX + 2, MARGIN + 2, MENU_BUTTON_SIZE, MENU_BUTTON_SIZE, HUD_SHADOW);
    fillRectToStrip(pixels, screenY, height, buttonX, MARGIN, MENU_BUTTON_SIZE, MENU_BUTTON_SIZE, HUD_BUTTON);
    drawRectToStrip(pixels, screenY, height, buttonX, MARGIN, MENU_BUTTON_SIZE, MENU_BUTTON_SIZE, HUD_BORDER);

    constexpr int lineWidth = 18;
    constexpr int lineHeight = 3;
    const int lineX = buttonX + (MENU_BUTTON_SIZE - lineWidth) / 2;
    fillRectToStrip(pixels, screenY, height, lineX, MARGIN + 10, lineWidth, lineHeight, HUD_TEXT);
    fillRectToStrip(pixels, screenY, height, lineX, MARGIN + 16, lineWidth, lineHeight, HUD_TEXT);
    fillRectToStrip(pixels, screenY, height, lineX, MARGIN + 22, lineWidth, lineHeight, HUD_TEXT);
}

// 繪製明亮風格的下拉選單到目前條帶。
// pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶資訊。
void HudLayer::drawMenu(uint16_t *pixels, int screenY, int height) const {
    const int menuX = SCREEN_WIDTH - MARGIN - MENU_WIDTH;
    const int menuY = MARGIN + MENU_BUTTON_SIZE + 4;
    const int menuHeight = MENU_ITEM_HEIGHT * MENU_ITEM_COUNT;
    fillRectToStrip(pixels, screenY, height, menuX + 2, menuY + 2, MENU_WIDTH, menuHeight, HUD_SHADOW);
    fillRectToStrip(pixels, screenY, height, menuX, menuY, MENU_WIDTH, menuHeight, HUD_PANEL);
    drawRectToStrip(pixels, screenY, height, menuX, menuY, MENU_WIDTH, menuHeight, HUD_BORDER);

    for (int item = 0; item < MENU_ITEM_COUNT; ++item) {
        const int itemY = menuY + item * MENU_ITEM_HEIGHT;
        if (item == selectedItem) {
            fillRectToStrip(pixels, screenY, height, menuX + 2, itemY + 2, MENU_WIDTH - 4, MENU_ITEM_HEIGHT - 3, HUD_SELECTED);
        }
        if (item > 0) {
            fillRectToStrip(pixels, screenY, height, menuX + 5, itemY, MENU_WIDTH - 10, 1, HUD_BORDER);
        }
        drawTextToStrip(
            pixels,
            screenY,
            height,
            MENU_LABELS_ZH[item],
            MENU_LABELS_EN[item],
            menuX + 10,
            itemY + 6,
            MENU_WIDTH - 20,
            16,
            HUD_TEXT
        );
    }
}
