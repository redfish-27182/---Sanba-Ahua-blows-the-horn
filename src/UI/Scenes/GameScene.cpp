#include "UI/Scenes/GameScene.h"

// 準備遊戲畫面使用的渲染資源。
// 無輸入；回傳：bool，是否成功。
bool GameScene::initialize() {
    return renderer.initialize();
}

// 利用相鄰觸控座標計算位移，避免重複累加起點位移。
// event：const InputEvent&，觸控事件。
void GameScene::handleInput(const InputEvent &event) {

    // 觸控按下時記錄起點座標，並啟動拖曳模式。
    if (event.type == InputEventType::TOUCH_DOWN) {
        lastX = event.x; 
        lastY = event.y; 
        dragging = true;
    } 

    // 拖曳事件時計算位移，並限制鏡頭在地圖內。
    else if (event.type == InputEventType::DRAG) {
        // 若尚未啟動拖曳模式則以起點座標為基準。
        if (!dragging) { 
            lastX = event.startX; 
            lastY = event.startY; 
            dragging = true; 
        }
        // 計算位移並限制鏡頭在地圖內，若鏡頭位置改變則標記需要重畫。
        if (camera.drag(  
                event.x - lastX, 
                event.y - lastY, 
                map.width  * 16, 
                map.height * 16 )
            ) {
            dirty = true;
        }
        lastX = event.x; 
        lastY = event.y;
    } 
    else { 
        dragging = false; 
    }
}

// 地圖或鏡頭改變後重畫整個可見區域。
// 回傳：bool，是否成功。
bool GameScene::drawIfNeeded() {
    // 若不需要重畫則直接回傳成功。
    if (!dirty) return true;

    // 以小型緩衝區繪製鏡頭內的圖層，若失敗則回傳失敗。
    if (!renderer.draw(map, camera, display)) return false;

    dirty = false;
    return true;
}
