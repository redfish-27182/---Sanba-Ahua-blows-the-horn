#pragma once
#include <Arduino.h>

class Camera {
public:
    int x = 300, y = 150;
    // 依手指位移移動鏡頭，限制在地圖內。
    // dx、dy：int，手指位移；mapWidth、mapHeight：int，地圖像素大小。
    // 回傳：bool，原點是否改變。
    bool drag(int dx, int dy, int mapWidth, int mapHeight);
};
