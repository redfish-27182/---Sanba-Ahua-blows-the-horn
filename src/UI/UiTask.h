#pragma once

#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>

#include "UI/PromptDialog.h"

// UiTask 是正常運行期間唯一持續操作 TFT 的 Task。
void UiTask_Start(TFT_eSPI &tft, PromptDialog &dialog, UBaseType_t priority, BaseType_t core);
