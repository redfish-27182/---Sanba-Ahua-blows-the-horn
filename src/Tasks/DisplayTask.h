#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

void DisplayTask_Start(TFT_eSPI &display, XPT2046_Touchscreen &touchscreen, UBaseType_t priority, BaseType_t coreID);