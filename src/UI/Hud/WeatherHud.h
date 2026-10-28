#pragma once

#include "App/AppMessages.h"
#include "UI/Hud/HudComponent.h"

// 左上角時間、天氣與溫度元件。
class WeatherHud : public HudComponent {
public:
    // 保存最新天氣資料。
    // state：const WeatherState&，已驗證的最新天氣；回傳：bool，是否需要重繪。
    bool setWeather(const WeatherState &state);

    // 處理天氣面板開始的觸控，避免拖曳地圖。
    // event：const InputEvent&，觸控事件；回傳：bool，是否攔截。
    bool handleInput(const InputEvent &event) override;

    // 每秒確認當地時間是否跨分鐘。
    // nowMs：uint32_t，目前 millis；回傳：bool，是否需要重繪。
    bool update(uint32_t nowMs) override;

    // 將天氣面板合成到目前條帶。
    // canvas：HudCanvas&，本次條帶的繪圖工具。
    void compose(HudCanvas &canvas) const override;

private:
    static constexpr int PANEL_HEIGHT = 28;
    static constexpr int CORNER_RADIUS = 2;
    static constexpr int TEXT_PADDING = 8;
    static constexpr int SECTION_GAP = 10;
    static constexpr int TEMPERATURE_LETTER_SPACING = -4;

    bool hasWeather = false;
    bool capturingTouch = false;
    uint32_t nextClockCheckAt = 0;
    int32_t lastClockMinute = -1;
    WeatherState weather{};

    // 判斷座標是否位於天氣面板。
    // x、y：int，螢幕座標；回傳：bool，是否命中。
    bool contains(int x, int y) const;

    // 計算目前天氣文字所需的面板寬度。
    // 無輸入；回傳：int，包含左右相同內距的面板寬度。
    int panelWidth() const;
};
