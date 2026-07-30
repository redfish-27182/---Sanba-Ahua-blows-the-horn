#include "UI/PromptDialog.h"

PromptDialog::PromptDialog(TFT_eSPI &tftScreen, XPT2046_Touchscreen &touchScreen) 
    : _tft(tftScreen), _touch(touchScreen) {}

void PromptDialog::drawWindow(const String &title, const String &message, 
                              const String &confirmText, const String &cancelText, 
                              bool highlightA, bool highlightB) {
    
    int winW = 250;
    int winH = 150;
    int winX = (320 - winW) / 2;
    int winY = (240 - winH) / 2;

    // 1. 底框
    _tft.fillRect(winX, winY, winW, winH, TFT_BLACK);
    _tft.drawRect(winX, winY, winW, winH, TFT_WHITE);

    // 2. 標題
    _tft.fillRect(winX, winY, winW, 28, TFT_NAVY);
    _tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    _tft.setTextDatum(TC_DATUM);
    _tft.drawString(title.c_str(), winX + winW / 2, winY + 6, 2);

    // 3. 內文
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextDatum(TL_DATUM);

    int startX = winX + 12;
    int startY = winY + 36;
    int lineHeight = 18;
    int currentY = startY;

    String tempLine = "";
    for (int i = 0; i < message.length(); i++) {
        char c = message.charAt(i);
        if (c == '\n') {
            _tft.drawString(tempLine.c_str(), startX, currentY, 2);
            tempLine = "";
            currentY += lineHeight;
        } else {
            tempLine += c;
            if (_tft.textWidth(tempLine.c_str(), 2) > (winW - 24)) {
                _tft.drawString(tempLine.c_str(), startX, currentY, 2);
                tempLine = "";
                currentY += lineHeight;
            }
        }
    }
    if (tempLine.length() > 0) {
        _tft.drawString(tempLine.c_str(), startX, currentY, 2);
    }

    // 4. 按鈕 A ([A] OK)
    int btnY = winY + winH - 38;
    int btnW = 110;
    int btnH = 28;

    if (highlightA) {
        _tft.fillRect(winX + 10, btnY, btnW, btnH, TFT_GREEN);
        _tft.setTextColor(TFT_BLACK, TFT_GREEN);
    } else {
        _tft.fillRect(winX + 10, btnY, btnW, btnH, TFT_DARKGREY);
        _tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    }
    _tft.drawRect(winX + 10, btnY, btnW, btnH, TFT_WHITE);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(confirmText.c_str(), winX + 10 + (btnW / 2), btnY + (btnH / 2), 2);

    // 5. 按鈕 B ([B] CANCEL)
    int btnBX = winX + winW - 120;
    if (highlightB) {
        _tft.fillRect(btnBX, btnY, btnW, btnH, TFT_RED);
        _tft.setTextColor(TFT_WHITE, TFT_RED);
    } else {
        _tft.fillRect(btnBX, btnY, btnW, btnH, TFT_DARKGREY);
        _tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    }
    _tft.drawRect(btnBX, btnY, btnW, btnH, TFT_WHITE);
    _tft.drawString(cancelText.c_str(), btnBX + (btnW / 2), btnY + (btnH / 2), 2);
}

bool PromptDialog::show(const String &title, const String &message, 
                        const String &confirmText, const String &cancelText) {
    
    drawWindow(title, message, confirmText, cancelText, false, false);

    delay(100);

    TS_Point p;
    bool selectedChoice = false;
    bool validSelection = false;

    int winW = 250;
    int winH = 150;
    int winX = (320 - winW) / 2;
    int winY = (240 - winH) / 2;

    int btnY = winY + winH - 38;
    int btnW = 110;
    int btnBX = winX + winW - 120;

    // 🎯 核心：這裡完全使用你測試範例中一模一樣的偵測迴圈！
    while (!validSelection) {
        
        if (_touch.touched()) {
            p = _touch.getPoint();

            // 完全複製你測試成功的公式
            int x = map(p.x, 200, 3700, 0, 320);
            int y = map(p.y, 240, 3800, 0, 240);

            x = constrain(x, 0, 320);
            y = constrain(y, 0, 240);

            Serial.printf("👉 [TOUCHED] Raw X: %d, Raw Y: %d -> Screen X: %d, Screen Y: %d\n", p.x, p.y, x, y);

            // 判斷按鈕 A 區域
            if (x >= (winX + 10) && x <= (winX + 10 + btnW) &&
                y >= btnY && y <= (btnY + 28)) {
                
                drawWindow(title, message, confirmText, cancelText, true, false);
                selectedChoice = true;
                validSelection = true;
                Serial.println("👉 按下 [A]");
            } 
            // 判斷按鈕 B 區域
            else if (x >= btnBX && x <= (btnBX + btnW) &&
                     y >= btnY && y <= (btnY + 28)) {
                
                drawWindow(title, message, confirmText, cancelText, false, true);
                selectedChoice = false;
                validSelection = true;
                Serial.println("👉 按下 [B]");
            }

            delay(30);
        }

        delay(10);
    }

    // 等待放開手指
    while (_touch.touched()) {
        delay(10);
    }
    delay(100);

    return selectedChoice;
}