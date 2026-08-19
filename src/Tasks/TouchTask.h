#pragma once
#include <Arduino.h>
#include <XPT2046_Touchscreen.h>

void TouchTask_Start(XPT2046_Touchscreen &touchscreen, UBaseType_t priority, BaseType_t coreID);