#pragma once
#include <TFT_eSPI.h>
#include "App/AppMessages.h"
#include "UI/PromptDialog.h"

// 由 UI Task 處理 MQTT 版本、詢問使用者並執行既有 OTA。
// event：const MqttInboundEvent&，訊息；tft：TFT_eSPI&，螢幕；dialog：PromptDialog&，對話框。
// 回傳：bool，是否顯示過提示或更新畫面。
bool UpdateManager_Handle(const MqttInboundEvent &event, TFT_eSPI &tft, PromptDialog &dialog);
