#ifndef PROMPT_DIALOG_H
#define PROMPT_DIALOG_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

class PromptDialog {
private:
    TFT_eSPI &_tft;
    XPT2046_Touchscreen &_touch;

    void drawWindow(const String &title, const String &message, 
                    const String &confirmText, const String &cancelText, 
                    bool highlightA, bool highlightB);

public:
    PromptDialog(TFT_eSPI &tftScreen, XPT2046_Touchscreen &touchScreen);

    // 🎯 這裡改成全大寫 OK / CANCEL 測試
    bool show(const String &title, const String &message, 
              const String &confirmText = "[A] OK", 
              const String &cancelText = "[B] CANCEL");
};

#endif // PROMPT_DIALOG_H