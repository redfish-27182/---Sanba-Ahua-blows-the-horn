#pragma once

#include "App/AppMessages.h"
#include "Display/ScreenOverlay.h"
#include "EventTypes.h"
#include "UI/Hud/MenuHud.h"
#include "UI/Hud/WeatherHud.h"

// 組裝 HUD 元件、維持繪製順序，並將輸入分流給最上層元件。
class HudComposer : public ScreenOverlay {
public:
    // 將天氣狀態交給對應 HUD 元件。
    // weather：const WeatherState&，目前天氣狀態；回傳：bool，是否需要重繪。
    bool setWeather(const WeatherState &weather);

    // 由上層以視覺反向順序分流觸控。
    // event：const InputEvent&，觸控事件；回傳：bool，是否由任一 HUD 攔截。
    bool handleInput(const InputEvent &event);

    // 更新需要定時變化的 HUD 元件。
    // nowMs：uint32_t，目前 millis；回傳：bool，是否需要重繪。
    bool update(uint32_t nowMs);

    // 將所有 HUD 元件依視覺順序合成到條帶。
    // pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶螢幕位置與高度。
    void composeStrip(uint16_t *pixels, int screenY, int height) const override;

    // 取得並清除互動元件造成的重繪要求。
    // 無輸入；回傳：bool，是否需要重繪。
    bool takeRedrawRequest();

private:
    WeatherHud weather;
    MenuHud menu;
    bool redrawRequested = false;

    // 收集單一元件的互動重繪要求。
    // component：HudComponent&，已處理事件的元件。
    void collectRedrawRequest(HudComponent &component);
};
