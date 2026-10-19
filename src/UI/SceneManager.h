#pragma once
#include "Map/MapLoader.h"
#include "UI/Scenes/GameScene.h"
#include "UI/Scenes/LoadingScene.h"
#include <memory>

class SceneManager {
public:
    // 建立畫面管理器並借用已初始化的螢幕。。
    explicit SceneManager(DisplayDriver &display) : display(display) {}

    // 顯示載入畫面並從 SD 卡載入遊戲資料。
    // 回傳：bool，是否成功進入遊戲。
    bool startGame();

    // 將觸控事件交給遊戲畫面。
    void handleInput(const InputEvent &event);

    // 更新需要重畫的畫面。
    void update();

    // 對話框或 OTA 結束後要求重畫。
    void redraw();
    
private:
    DisplayDriver &display;
    LoadingScene loading;
    MapLoader loader;
    MapData map;
    std::unique_ptr<GameScene> game;
    const char *failureMessage = nullptr;
};
