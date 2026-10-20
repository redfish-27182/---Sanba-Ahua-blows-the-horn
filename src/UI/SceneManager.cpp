#include "UI/SceneManager.h"
#include <new>

// 先顯示載入狀態，再同步讀取 SD 卡並建立遊戲畫面。
// 無輸入；回傳：bool，是否成功。
bool SceneManager::startGame() {

    // 顯示載入中訊息。
    loading.show(display, "Loading game...");

    // 從 SD 卡載入地圖與素材，失敗時顯示錯誤訊息。
    if (!loader.load("/game/maps/first_map.bin", "/game/tilesets/tileset.bin", map)) {
        failureMessage = "Map load failed. Check SD card.";
        loading.show(display, failureMessage);
        return false;
    }

    // 建立遊戲畫面，失敗時顯示錯誤訊息。
    Serial.printf(
        "[GameScene] allocate %u bytes; free heap=%u, largest block=%u\n",
        static_cast<unsigned>(sizeof(GameScene)), // 需要分配的記憶體大小
        ESP.getFreeHeap(),    // 取得可用的堆積記憶體大小
        ESP.getMaxAllocHeap() // 取得可一次分配的最大堆積記憶體區塊大小
    );
    
    game.reset(new (std::nothrow) GameScene(map, display));
    if (!game) { 
        Serial.printf(
            "[GameScene] allocation failed; free heap=%u, largest block=%u\n",
            ESP.getFreeHeap(), 
            ESP.getMaxAllocHeap()
        );
        failureMessage = "Not enough memory for game."; 
        loading.show(display, failureMessage); 
        return false; 
    }
    Serial.printf(
        "[GameScene] allocation succeeded; free heap=%u, largest block=%u\n",
        ESP.getFreeHeap(), 
        ESP.getMaxAllocHeap()
    );

    if (!game->initialize()) {
        game.reset();
        failureMessage = "Not enough memory to render.";
        loading.show(display, failureMessage);
        return false;
    }
    // 建立完成，清除錯誤訊息。
    failureMessage = nullptr;
    return true;
}

// 將觸控事件交給遊戲畫面。
// event：const InputEvent&，觸控事件；
void SceneManager::handleInput(const InputEvent &event) { 
    if (game) {
        game->handleInput(event); 
    }
}

// 畫面需要更新時同步繪製；失敗顯示錯誤。
void SceneManager::update() {
    if (game && !game -> drawIfNeeded()) { 
        game.reset(); 
        failureMessage = "Not enough memory to render."; 
        loading.show(display, failureMessage); 
    }
}

// 提示視窗結束後恢復畫面。
// 無輸入及回傳值。
void SceneManager::redraw() {
    if (game) game->requestRedraw();
    else if (failureMessage) loading.show(display, failureMessage);
}
