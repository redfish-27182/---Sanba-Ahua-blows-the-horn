#include "UI/Hud/MenuHud.h"

#include "Config.h"
#include "UI/Hud/HudCanvas.h"
#include "UI/Hud/HudStyle.h"

namespace {
constexpr const char *MENU_LABELS_ZH[] = {"設定", "資訊", "說明"};
constexpr const char *MENU_LABELS_EN[] = {"SETTINGS", "INFO", "HELP"};
}

// 依觸控事件開關選單或選取項目。
// event：const InputEvent&，觸控事件；回傳：bool，是否由選單攔截。
bool MenuHud::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TOUCH_DOWN) {
        if (menuOpen || isButton(event.x, event.y)) {
            capturingTouch = true;
            return true;
        }
        return false;
    }
    if (!capturingTouch) return false;

    if (event.type == InputEventType::TAP) {
        if (isButton(event.x, event.y)) {
            menuOpen = !menuOpen;
            selectedItem = -1;
            redrawRequested = true;
        } else if (menuOpen) {
            const int item = itemAt(event.x, event.y);
            if (item >= 0) {
                selectedItem = item;
            } else {
                menuOpen = false;
                selectedItem = -1;
            }
            redrawRequested = true;
        }
        capturingTouch = false;
    } else if (event.type == InputEventType::RELEASE
        || event.type == InputEventType::LONG_PRESS
        || event.type == InputEventType::SWIPE) {
        capturingTouch = false;
    }
    return true;
}

// 取得並清除選單互動造成的重繪要求。
// 無輸入；回傳：bool，是否需要重繪。
bool MenuHud::takeRedrawRequest() {
    const bool requested = redrawRequested;
    redrawRequested = false;
    return requested;
}

// 判斷座標是否位於右上選單按鈕。
// x、y：int，螢幕座標；回傳：bool，是否命中。
bool MenuHud::isButton(int x, int y) const {
    const int buttonX = SCREEN_WIDTH - HudStyle::MARGIN - BUTTON_SIZE;
    return x >= buttonX && x < buttonX + BUTTON_SIZE
        && y >= HudStyle::MARGIN && y < HudStyle::MARGIN + BUTTON_SIZE;
}

// 取得下拉選單中被點選的項目索引。
// x、y：int，螢幕座標；回傳：int，0 到 2 或 -1。
int MenuHud::itemAt(int x, int y) const {
    const int menuX = SCREEN_WIDTH - HudStyle::MARGIN - MENU_WIDTH;
    const int menuY = HudStyle::MARGIN + BUTTON_SIZE + 4;
    const int menuHeight = ITEM_HEIGHT * ITEM_COUNT;
    if (x < menuX || x >= menuX + MENU_WIDTH || y < menuY || y >= menuY + menuHeight) {
        return -1;
    }
    return (y - menuY) / ITEM_HEIGHT;
}

// 將按鈕與展開中的選單依視覺順序合成。
// canvas：HudCanvas&，本次條帶的繪圖工具。
void MenuHud::compose(HudCanvas &canvas) const {
    drawButton(canvas);
    if (menuOpen) drawMenu(canvas);
}

// 繪製右上角三橫線選單按鈕。
// canvas：HudCanvas&，本次條帶的繪圖工具。
void MenuHud::drawButton(HudCanvas &canvas) const {
    const int buttonX = SCREEN_WIDTH - HudStyle::MARGIN - BUTTON_SIZE;
    canvas.fillRect(buttonX + 2, HudStyle::MARGIN + 2, BUTTON_SIZE, BUTTON_SIZE, HudStyle::SHADOW);
    canvas.fillRect(buttonX, HudStyle::MARGIN, BUTTON_SIZE, BUTTON_SIZE, HudStyle::BUTTON);
    canvas.drawRect(buttonX, HudStyle::MARGIN, BUTTON_SIZE, BUTTON_SIZE, HudStyle::BORDER);

    constexpr int lineWidth = 18;
    constexpr int lineHeight = 3;
    const int lineX = buttonX + (BUTTON_SIZE - lineWidth) / 2;
    canvas.fillRect(lineX, HudStyle::MARGIN + 10, lineWidth, lineHeight, HudStyle::TEXT);
    canvas.fillRect(lineX, HudStyle::MARGIN + 16, lineWidth, lineHeight, HudStyle::TEXT);
    canvas.fillRect(lineX, HudStyle::MARGIN + 22, lineWidth, lineHeight, HudStyle::TEXT);
}

// 繪製明亮風格的下拉選單。
// canvas：HudCanvas&，本次條帶的繪圖工具。
void MenuHud::drawMenu(HudCanvas &canvas) const {
    const int menuX = SCREEN_WIDTH - HudStyle::MARGIN - MENU_WIDTH;
    const int menuY = HudStyle::MARGIN + BUTTON_SIZE + 4;
    const int menuHeight = ITEM_HEIGHT * ITEM_COUNT;
    canvas.fillRect(menuX + 2, menuY + 2, MENU_WIDTH, menuHeight, HudStyle::SHADOW);
    canvas.fillRect(menuX, menuY, MENU_WIDTH, menuHeight, HudStyle::PANEL);
    canvas.drawRect(menuX, menuY, MENU_WIDTH, menuHeight, HudStyle::BORDER);

    for (int item = 0; item < ITEM_COUNT; ++item) {
        const int itemY = menuY + item * ITEM_HEIGHT;
        if (item == selectedItem) {
            canvas.fillRect(menuX + 2, itemY + 2, MENU_WIDTH - 4, ITEM_HEIGHT - 3, HudStyle::SELECTED);
        }
        if (item > 0) {
            canvas.fillRect(menuX + 5, itemY, MENU_WIDTH - 10, 1, HudStyle::BORDER);
        }
        canvas.drawText(
            MENU_LABELS_ZH[item], MENU_LABELS_EN[item], menuX + 10, itemY + 6,
            MENU_WIDTH - 20, 16, HudStyle::TEXT
        );
    }
}
