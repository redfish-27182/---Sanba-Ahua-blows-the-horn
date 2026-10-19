#pragma once

#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>

#include "UI/PromptDialog.h"

// UiTask ，啟動!!! 。
void UiTask_Start(
    TFT_eSPI &tft, 
    PromptDialog &dialog, 
    UBaseType_t priority, 
    BaseType_t core
);
