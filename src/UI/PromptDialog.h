#ifndef PROMPT_DIALOG_H
#define PROMPT_DIALOG_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class PromptDialog {
    
    private:
        TFT_eSPI &_tft;

        void drawWindow(
            const String &title, 
            const String &message, 
            const String &confirmText, 
            const String &cancelText, 
            bool highlightA, 
            bool highlightB
        );

    public:
        // 觸控輸入由 TouchTask 統一處理；此類別只負責繪製與判斷事件座標。
        PromptDialog(TFT_eSPI &tftScreen);

        // 🎯 這裡改成全大寫 OK / CANCEL 測試
        bool show(
            const String &title, 
            const String &message, 
            const String &confirmText = "[A] OK", 
            const String &cancelText = "[B] CANCEL"
        );
};

#endif // PROMPT_DIALOG_H
