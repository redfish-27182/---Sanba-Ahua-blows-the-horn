#pragma once

#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>

// UpdateManager Task：接收 MQTT 版本、詢問使用者，並交給既有 GitHubOTA 更新。
void UpdateManager_Start(TFT_eSPI &tft, UBaseType_t priority, BaseType_t core);
