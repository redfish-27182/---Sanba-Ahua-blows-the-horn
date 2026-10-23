#include "Game/Camera.h"
#include "Config.h"

// 手指拖曳時反向移動鏡頭，較小的地圖固定於原點。
// dx、dy：int，拖曳位移；mapWidth、mapHeight：int，地圖像素大小。
// 回傳：bool，原點是否改變。
bool Camera::drag(int dx, int dy, int mapWidth, int mapHeight) {
    const int oldX = x, oldY = y;
    x = constrain(x - dx, 0, max(0, mapWidth - SCREEN_WIDTH));
    y = constrain(y - dy, 0, max(0, mapHeight - SCREEN_HEIGHT));
    return x != oldX || y != oldY;
}
