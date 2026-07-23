#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1

/* 記憶體配置 */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (32U * 1024U)

/* 啟用核心元件 */
#define LV_USE_LOG 0
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_LIST 1
#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 1

/* 🎯 核心：必須確保字型開啟！ */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14 // 👈 設定預設字型

#endif