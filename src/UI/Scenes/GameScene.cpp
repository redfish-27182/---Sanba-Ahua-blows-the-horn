#include "UI/Scenes/GameScene.h"

// 利用相鄰觸控座標計算位移，避免重複累加起點位移。
// event：const InputEvent&，觸控事件；無回傳值。
void GameScene::handleInput(const InputEvent &event) {
    if (event.type == InputEventType::TOUCH_DOWN) {
        lastX = event.x; 
        lastY = event.y; 
        dragging = true;
    } 
    else if (event.type == InputEventType::DRAG) {
        if (!dragging) { 
            lastX = event.startX; 
            lastY = event.startY; 
            dragging = true; 
        }
        if (camera.drag(event.x - lastX, event.y - lastY, map.width * 16, map.height * 16)) dirty = true;
        lastX = event.x; 
        lastY = event.y;
    } 
    else { 
        dragging = false; 
    }
}

// 地圖或鏡頭改變後重畫整個可見區域。
// 無輸入；回傳：bool，是否成功。
bool GameScene::drawIfNeeded() {
    if (!dirty) return true;
    if (!renderer.draw(map, camera, display)) return false;
    dirty = false;
    return true;
}
