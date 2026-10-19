#pragma once
#include "Display/DisplayDriver.h"

class LoadingScene {
public:
    // 顯示載入中或載入錯誤。
    // display：DisplayDriver&，螢幕；message：const char*，文字；無回傳值。
    void show(DisplayDriver &display, const char *message) { display.showMessage(message); }
};
