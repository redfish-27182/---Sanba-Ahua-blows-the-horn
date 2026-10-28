#include "UI/Hud/WeatherHud.h"

#include <cstdio>
#include <ctime>

#include "UI/Hud/HudCanvas.h"
#include "UI/Hud/HudStyle.h"

namespace {
// 將 WeatherAPI 天氣代碼轉成第一版 HUD 使用的簡短中文。
// conditionCode：uint16_t，WeatherAPI 天氣代碼；回傳：const char*，晴、陰或雨。
const char *conditionLabel(uint16_t conditionCode) {
    if (conditionCode == 1000) return "晴";
    if (conditionCode == 1003 || conditionCode == 1006 || conditionCode == 1009
        || conditionCode == 1030 || conditionCode == 1135 || conditionCode == 1147) {
        return "陰";
    }
    return "雨";
}

// 以全域 UTC 時鐘和後端偏移量換算設備所在地的時間。
// utcOffsetMinutes：int16_t，UTC 偏移分鐘；output：tm&，換算後本地時間；回傳：bool，時間是否可用。
bool localTimeFromUtcOffset(int16_t utcOffsetMinutes, tm &output) {
    const time_t utcNow = time(nullptr);
    if (utcNow < 1600000000) return false;
    const time_t localNow = utcNow + time_t(utcOffsetMinutes) * 60;
    return gmtime_r(&localNow, &output) != nullptr;
}

// 計算 UTF-8 字串內的字形數量，不把 UTF-8 延續位元組算成另一個字。
// text：const char*，UTF-8 字串；回傳：int，字形數量。
int utf8GlyphCount(const char *text) {
    int count = 0;
    while (*text) {
        const uint8_t byte = static_cast<uint8_t>(*text++);
        if ((byte & 0xC0) != 0x80) ++count;
    }
    return count;
}

// 計算縮小字距後的實際像素寬度。
// text：const char*，UTF-8 字串；letterSpacing：int，字距調整像素；回傳：int，像素寬度。
int compactTextWidth(const char *text, int letterSpacing) {
    const int glyphCount = utf8GlyphCount(text);
    if (glyphCount < 1) return 0;
    return HudStyle::GLYPH_WIDTH
        + (glyphCount - 1) * (HudStyle::GLYPH_WIDTH + letterSpacing);
}

// 產生 HUD 顯示的整數溫度文字。
// temperatureTenthsC：int16_t，攝氏溫度乘以十；output：char*，輸出的 UTF-8 文字。
void formatTemperature(int16_t temperatureTenthsC, char *output, size_t outputSize) {
    const int roundedTemperature = temperatureTenthsC >= 0
        ? (temperatureTenthsC + 5) / 10
        : (temperatureTenthsC - 5) / 10;
    snprintf(output, outputSize, "%d°C", roundedTemperature);
}
}

// 保存最新天氣，顯示文字有變化時要求遊戲重新合成畫面。
// state：const WeatherState&，已驗證天氣狀態；回傳：bool，是否需要重繪。
bool WeatherHud::setWeather(const WeatherState &state) {
    const bool changed = !hasWeather
        || weather.temperatureTenthsC != state.temperatureTenthsC
        || weather.conditionCode != state.conditionCode
        || weather.utcOffsetMinutes != state.utcOffsetMinutes;
    weather = state;
    hasWeather = true;
    return changed;
}

// 攔截從天氣面板開始的觸控，避免觸碰 HUD 時拖曳地圖。
// event：const InputEvent&，觸控事件；回傳：bool，是否攔截。
bool WeatherHud::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TOUCH_DOWN) {
        capturingTouch = contains(event.x, event.y);
        return capturingTouch;
    }
    if (!capturingTouch) return false;

    if (event.type == InputEventType::TAP
        || event.type == InputEventType::RELEASE
        || event.type == InputEventType::LONG_PRESS
        || event.type == InputEventType::SWIPE) {
        capturingTouch = false;
    }
    return true;
}

// 低頻檢查本地分鐘，分鐘變化時才要求遊戲重繪 HUD。
// nowMs：uint32_t，目前 millis；回傳：bool，是否需要重繪。
bool WeatherHud::update(uint32_t nowMs) {
    if (static_cast<int32_t>(nowMs - nextClockCheckAt) < 0) return false;
    nextClockCheckAt = nowMs + 1000;
    if (!hasWeather) return false;

    const time_t utcNow = time(nullptr);
    if (utcNow < 1600000000) return false;
    const time_t localNow = utcNow + time_t(weather.utcOffsetMinutes) * 60;
    const int32_t currentMinute = static_cast<int32_t>(localNow / 60);
    if (currentMinute == lastClockMinute) return false;

    lastClockMinute = currentMinute;
    return true;
}

// 判斷座標是否位於目前寬度的天氣面板。
// x、y：int，螢幕座標；回傳：bool，是否命中。
bool WeatherHud::contains(int x, int y) const {
    return x >= HudStyle::MARGIN && x < HudStyle::MARGIN + panelWidth()
        && y >= HudStyle::MARGIN && y < HudStyle::MARGIN + PANEL_HEIGHT;
}

// 計算目前字串需要的面板寬度，確保左右文字內距相同。
// 無輸入；回傳：int，面板像素寬度。
int WeatherHud::panelWidth() const {
    char temperatureText[16]{};
    if (hasWeather) {
        formatTemperature(weather.temperatureTenthsC, temperatureText, sizeof(temperatureText));
    } else {
        snprintf(temperatureText, sizeof(temperatureText), "--°C");
    }

    const int contentWidth = HudStyle::GLYPH_WIDTH * 5
        + SECTION_GAP
        + HudStyle::GLYPH_WIDTH
        + SECTION_GAP
        + HudStyle::GLYPH_WIDTH
        + SECTION_GAP
        + compactTextWidth(temperatureText, TEMPERATURE_LETTER_SPACING);
    return contentWidth + TEXT_PADDING * 2;
}

// 將左上角天氣資訊合成到目前條帶。
// canvas：HudCanvas&，本次條帶的繪圖工具。
void WeatherHud::compose(HudCanvas &canvas) const {
    const int width = panelWidth();
    canvas.fillRoundedRect(
        HudStyle::MARGIN + 2, HudStyle::MARGIN + 2,
        width, PANEL_HEIGHT, CORNER_RADIUS, HudStyle::SHADOW
    );
    canvas.fillRoundedRect(
        HudStyle::MARGIN, HudStyle::MARGIN,
        width, PANEL_HEIGHT, CORNER_RADIUS, HudStyle::PANEL
    );
    canvas.drawRoundedRect(
        HudStyle::MARGIN, HudStyle::MARGIN,
        width, PANEL_HEIGHT, CORNER_RADIUS, HudStyle::BORDER
    );

    char timeText[8]{};
    char weatherText[8]{};
    char temperatureText[16]{};
    tm localTime{};
    if (hasWeather && localTimeFromUtcOffset(weather.utcOffsetMinutes, localTime)) {
        snprintf(timeText, sizeof(timeText), "%02d:%02d", localTime.tm_hour, localTime.tm_min);
        snprintf(weatherText, sizeof(weatherText), "%s", conditionLabel(weather.conditionCode));
        formatTemperature(weather.temperatureTenthsC, temperatureText, sizeof(temperatureText));
    } else {
        snprintf(timeText, sizeof(timeText), "--:--");
        snprintf(weatherText, sizeof(weatherText), "□");
        snprintf(temperatureText, sizeof(temperatureText), "--°C");
    }

    int textX = HudStyle::MARGIN + TEXT_PADDING;
    canvas.drawText(timeText, "--:--", textX, HudStyle::MARGIN + 6, HudStyle::GLYPH_WIDTH * 5, 16, HudStyle::TEXT);
    textX += HudStyle::GLYPH_WIDTH * 5 + SECTION_GAP;
    canvas.drawText(weatherText, "?", textX, HudStyle::MARGIN + 6, HudStyle::GLYPH_WIDTH, 16, HudStyle::TEXT);
    textX += HudStyle::GLYPH_WIDTH + SECTION_GAP;
    canvas.drawText("/", "/", textX, HudStyle::MARGIN + 6, HudStyle::GLYPH_WIDTH, 16, HudStyle::TEXT);
    textX += HudStyle::GLYPH_WIDTH + SECTION_GAP;
    canvas.drawText(
        temperatureText, "--C", textX, HudStyle::MARGIN + 6,
        compactTextWidth(temperatureText, TEMPERATURE_LETTER_SPACING), 16,
        HudStyle::TEXT, TEMPERATURE_LETTER_SPACING
    );
}
