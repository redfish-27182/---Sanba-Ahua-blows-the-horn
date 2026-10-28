#pragma once

#include "UI/Hud/HudComponent.h"

// 右上角按鈕與下拉選單元件。
class MenuHud : public HudComponent {
public:
    // 依觸控事件開關選單或選取項目。
    // event：const InputEvent&，觸控事件；回傳：bool，是否由選單攔截。
    bool handleInput(const InputEvent &event) override;

    // 將按鈕與展開選單合成到目前條帶。
    // canvas：HudCanvas&，本次條帶的繪圖工具。
    void compose(HudCanvas &canvas) const override;

    // 取得並清除選單互動造成的重繪要求。
    // 無輸入；回傳：bool，是否需要重繪。
    bool takeRedrawRequest() override;

private:
    static constexpr int BUTTON_SIZE = 36;
    static constexpr int MENU_WIDTH = 112;
    static constexpr int ITEM_HEIGHT = 28;
    static constexpr int ITEM_COUNT = 3;

    bool menuOpen = false;
    bool capturingTouch = false;
    bool redrawRequested = false;
    int selectedItem = -1;

    // 判斷座標是否位於右上選單按鈕。
    // x、y：int，螢幕座標；回傳：bool，是否命中。
    bool isButton(int x, int y) const;

    // 取得下拉選單中被點選的項目索引。
    // x、y：int，螢幕座標；回傳：int，0 到 2 或 -1。
    int itemAt(int x, int y) const;

    // 繪製右上角三橫線按鈕。
    // canvas：HudCanvas&，本次條帶的繪圖工具。
    void drawButton(HudCanvas &canvas) const;

    // 繪製展開中的下拉選單。
    // canvas：HudCanvas&，本次條帶的繪圖工具。
    void drawMenu(HudCanvas &canvas) const;
};
