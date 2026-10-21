#pragma once
#include <TFT_eSPI.h>

// 開機載入系統字庫，失敗時畫面改用英文備用文字。
// 回傳：bool，是否成功載入。
bool SystemText_Load();

// 依字庫是否可用選擇中文字串或英文備用字串。
// zh、en：const char*，兩種文字；回傳：const char*，目前可顯示的文字。
const char *SystemText_Choose(const char *zh, const char *en);

// 於指定範圍繪製中文或英文備用文字。
// screen：螢幕；zh、en：中文及備用文字；x、y、width、height：區域；color：顏色；center：置中。
void SystemText_Draw(TFT_eSPI &screen, const char *zh, const char *en, int x, int y, int width, int height, uint16_t color, bool center = false);
