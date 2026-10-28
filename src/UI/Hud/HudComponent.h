#pragma once

#include <Arduino.h>

#include "EventTypes.h"

class HudCanvas;

// 所有 HUD 元件共用的最小介面，可依需求覆寫輸入與定時更新。
class HudComponent {
public:
    virtual ~HudComponent() = default;

    // 將元件寫入目前的條帶緩衝區。
    // canvas：HudCanvas&，本次條帶的繪圖工具。
    virtual void compose(HudCanvas &canvas) const = 0;

    // 處理觸控事件；預設不攔截，讓沒有互動功能的元件不必實作。
    // event：const InputEvent&，觸控事件；回傳：bool，是否攔截。
    virtual bool handleInput(const InputEvent &event) { (void)event; return false; }

    // 執行低頻狀態更新；預設沒有定時工作。
    // nowMs：uint32_t，目前 millis；回傳：bool，是否需要重繪。
    virtual bool update(uint32_t nowMs) { (void)nowMs; return false; }

    // 取得並清除互動造成的重繪要求；預設沒有要求。
    // 無輸入；回傳：bool，是否需要重繪。
    virtual bool takeRedrawRequest() { return false; }
};
