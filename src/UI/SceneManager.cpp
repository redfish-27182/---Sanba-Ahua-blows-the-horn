#include "UI/SceneManager.h"
#include <new>
#include "Font/SystemText.h"
#include "Network/MqttTask.h"

// 先顯示載入狀態，再同步讀取 SD 卡並建立遊戲畫面。
// 無輸入；回傳：bool，是否成功。
bool SceneManager::startGame() {

    // 顯示載入中訊息。
    loading.show(display, SystemText_Choose("正在載入遊戲...", "Loading game..."));

    // 從 SD 卡載入地圖與素材，失敗時顯示錯誤訊息。
    if (!loader.load("/game/maps/first_map.bin", "/game/tilesets/tileset.bin", map)) {
        failureMessage = SystemText_Choose("地圖載入失敗，請檢查 SD 卡。", "Map load failed. Check SD card.");
        loading.show(display, failureMessage);
        return false;
    }

    // 建立遊戲畫面，失敗時顯示錯誤訊息。
    game.reset(new (std::nothrow) GameScene(map, display));
    if (!game) { 
        Serial.println("[GameScene] allocation failed");
        failureMessage = SystemText_Choose("記憶體不足，無法進入遊戲。", "Not enough memory for game."); 
        loading.show(display, failureMessage); 
        return false; 
    }

    if (!game->initialize()) {
        game.reset();
        failureMessage = SystemText_Choose("記憶體不足，無法顯示畫面。", "Not enough memory to render.");
        loading.show(display, failureMessage);
        return false;
    }
    // 地圖最大像素區塊已配置完成，此時才允許 Wi-Fi 掃描配置暫時記憶體。
    MqttTask_RequestBootWifiScan();
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
        failureMessage = SystemText_Choose("記憶體不足，無法顯示畫面。", "Not enough memory to render."); 
        loading.show(display, failureMessage); 
    }
}

// 提示視窗結束後恢復畫面。
// 無輸入及回傳值。
void SceneManager::redraw() {
    if (game) game->requestRedraw();
    else if (failureMessage) loading.show(display, failureMessage);
}
