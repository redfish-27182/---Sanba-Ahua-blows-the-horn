#include "UI/PromptDialog.h"
#include "../EventTypes.h"
#include "Font/SystemText.h"
#include "Config.h"

PromptDialog::PromptDialog(TFT_eSPI &tftScreen)
    : _tft(tftScreen) {}

void PromptDialog::drawWindow(
    const String &title,
    const String &message,
    const String &confirmText,
    const String &cancelText,
    bool highlightA,
    bool highlightB
) {
    constexpr int winW = 250;
    constexpr int winH = 150;
    constexpr int btnW = 110;
    constexpr int btnH = 28;
    const int winX = (SCREEN_WIDTH - winW) / 2;
    const int winY = (SCREEN_HEIGHT - winH) / 2;

    // 對話框外框與標題列。
    _tft.fillRect(winX, winY, winW, winH, TFT_BLACK);
    _tft.drawRect(winX, winY, winW, winH, TFT_WHITE);
    _tft.fillRect(winX, winY, winW, 28, TFT_NAVY);
    SystemText_Draw(_tft, title.c_str(), title.c_str(), winX, winY + 6, winW, 16, TFT_YELLOW, true);

    // 將訊息依視窗可用寬度分行繪製。
    const int startX = winX + 12;
    SystemText_Draw(_tft, message.c_str(), message.c_str(), startX, winY + 36, winW - 24, 70, TFT_WHITE);

    const int btnY = winY + winH - 38;
    const int btnAX = winX + 10;
    const int btnBX = winX + winW - 120;

    // A／確認按鈕。
    if (highlightA) {
        _tft.fillRect(btnAX, btnY, btnW, btnH, TFT_GREEN);
        _tft.setTextColor(TFT_BLACK, TFT_GREEN);
    } else {
        _tft.fillRect(btnAX, btnY, btnW, btnH, TFT_DARKGREY);
        _tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    }
    _tft.drawRect(btnAX, btnY, btnW, btnH, TFT_WHITE);
    SystemText_Draw(_tft, confirmText.c_str(), confirmText.c_str(), btnAX, btnY + 6, btnW, 16, highlightA ? TFT_BLACK : TFT_WHITE, true);

    // B／取消按鈕。
    if (highlightB) {
        _tft.fillRect(btnBX, btnY, btnW, btnH, TFT_RED);
        _tft.setTextColor(TFT_WHITE, TFT_RED);
    } else {
        _tft.fillRect(btnBX, btnY, btnW, btnH, TFT_DARKGREY);
        _tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    }
    _tft.drawRect(btnBX, btnY, btnW, btnH, TFT_WHITE);
    SystemText_Draw(_tft, cancelText.c_str(), cancelText.c_str(), btnBX, btnY + 6, btnW, 16, TFT_WHITE, true);
}

bool PromptDialog::show(
    const String &title,
    const String &message,
    const String &confirmText,
    const String &cancelText
) {
    constexpr int winW = 250;
    constexpr int winH = 150;
    constexpr int btnW = 110;
    constexpr int btnH = 28;
    const int winX = (SCREEN_WIDTH - winW) / 2;
    const int winY = (SCREEN_HEIGHT - winH) / 2;
    const int btnY = winY + winH - 38;
    const int btnAX = winX + 10;
    const int btnBX = winX + winW - 120;

    drawWindow(title, message, confirmText, cancelText, false, false);

    // TouchTask 尚未啟動或佇列建立失敗時，無法等待觸控事件。
    if (inputQueue == nullptr) {
        Serial.println("觸控佇列尚未建立，PromptDialog 無法正常運作。");
        return false;
    }

    // 對話框開啟前的觸控不應被誤當成按鈕操作。
    InputEvent evt;
    while (xQueueReceive(inputQueue, &evt, 0) == pdTRUE) {}
    const uint32_t openedAt = millis();

    for (;;) {
        // 等待 TouchTask 送來的事件。
        if (xQueueReceive(inputQueue, &evt, pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }

        // TAP 在手指放開後才送出，適合作為按鈕確認事件。
        // 第二個條件可防止剛開啟對話框前遺留的事件被使用。
        if (evt.type != InputEventType::TAP ||
            static_cast<int32_t>(evt.timestamp - openedAt) < 0) {
            continue;
        }

        if (evt.x >= btnAX && evt.x <= btnAX + btnW &&
            evt.y >= btnY && evt.y <= btnY + btnH) {
            drawWindow(title, message, confirmText, cancelText, true, false);
            delay(100); // 顯示已選取的按鈕回饋。
            return true;
        }

        if (evt.x >= btnBX && evt.x <= btnBX + btnW &&
            evt.y >= btnY && evt.y <= btnY + btnH) {
            drawWindow(title, message, confirmText, cancelText, false, true);
            delay(100); // 顯示已選取的按鈕回饋。
            return false;
        }
    }
}
