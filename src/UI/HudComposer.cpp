#include "UI/HudComposer.h"
#include "Config.h"
#include "UI/Hud/HudCanvas.h"

// 將天氣狀態交給天氣元件。
// state：const WeatherState&，目前天氣狀態；回傳：bool，是否需要重繪。
bool HudComposer::setWeather(const WeatherState &state) {
    return weather.setWeather(state);
}

// 以最上層元件優先的順序分流觸控。
// event：const InputEvent&，觸控事件；回傳：bool，是否由任一 HUD 攔截。
bool HudComposer::handleInput(const InputEvent &event) {
    // 選單可覆蓋其他元件，因此優先取得觸控。
    if (menu.handleInput(event)) {
        collectRedrawRequest(menu);
        return true;
    }
    if (weather.handleInput(event)) {
        collectRedrawRequest(weather);
        return true;
    }
    return false;
}

// 更新所有需要定時變化的 HUD 元件。
// nowMs：uint32_t，目前 millis；回傳：bool，是否需要重繪。
bool HudComposer::update(uint32_t nowMs) {
    const bool weatherChanged = weather.update(nowMs);
    const bool menuChanged = menu.update(nowMs);
    return weatherChanged || menuChanged;
}

// 依視覺層級把 HUD 元件寫入目前條帶。
// pixels：uint16_t*，RGB565 條帶；screenY、height：int，條帶螢幕位置與高度。
void HudComposer::composeStrip(uint16_t *pixels, int screenY, int height) const {
    HudCanvas canvas(pixels, SCREEN_WIDTH, screenY, height);
    weather.compose(canvas);
    menu.compose(canvas);
}

// 取得並清除元件互動造成的重繪要求。
// 無輸入；回傳：bool，是否需要重繪。
bool HudComposer::takeRedrawRequest() {
    const bool requested = redrawRequested;
    redrawRequested = false;
    return requested;
}

// 收集元件互動造成的重繪要求。
// component：HudComponent&，已處理事件的元件。
void HudComposer::collectRedrawRequest(HudComponent &component) {
    if (component.takeRedrawRequest()) {
        redrawRequested = true;
    }
}
