#ifndef PIXEL_WIFI_LVGL_H
#define PIXEL_WIFI_LVGL_H

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>

static char g_real_ssids[10][33]; 
static int g_real_quality[10];
static int g_real_count = 0;
static bool g_is_scanning = false;

class PixelWiFiLVGL {
private:
    lv_obj_t *status_label = NULL;

public:
    PixelWiFiLVGL() {}

    void initUI() {
        Serial.println("🛠️ [PixelWiFiLVGL] 初始化極簡 UI...");

        // 頂部狀態列
        status_label = lv_label_create(lv_scr_act());
        lv_label_set_text(status_label, "INIT SCANNER...");
        lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFF), 0);
    }

    // 🎯 1. 發起安全的背景非同步掃描
    void startAsyncScan() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);

        // async = true (非同步，不阻塞 CPU，不搶 LVGL 記憶體)
        WiFi.scanNetworks(true);
        g_is_scanning = true;

        if (status_label) {
            lv_label_set_text(status_label, "SCANNING WI-FI...");
        }
        Serial.println("📡 背景非同步 Wi-Fi 掃描已啟動...");
    }

    // 🎯 2. 檢查非同步掃描結果 (在 loop 中持續監聽)
    void checkScanComplete() {
        if (!g_is_scanning) return;

        int n = WiFi.scanComplete();

        // 掃描中會傳回 WIFI_SCAN_RUNNING (-1)
        if (n == WIFI_SCAN_RUNNING) return;

        // n >= 0 代表掃描順利完成！
        if (n >= 0) {
            Serial.printf("✅ [Scan Finished] 共找到 %d 個 AP 網路！\n", n);
            g_is_scanning = false;

            g_real_count = 0;
            if (n > 0) {
                int limit = (n > 5) ? 5 : n; // 抓前 5 個安全測試
                for (int i = 0; i < limit; i++) {
                    String rawSSID = WiFi.SSID(i);
                    int32_t rssi = WiFi.RSSI(i);

                    // 確保只取 ASCII 可列印字元
                    String cleanSSID = "";
                    for (size_t c = 0; c < rawSSID.length(); c++) {
                        char ch = rawSSID[c];
                        if (ch >= 32 && ch <= 126) cleanSSID += ch;
                    }
                    if (cleanSSID.length() == 0) cleanSSID = "Hidden_AP";

                    memset(g_real_ssids[g_real_count], 0, 33);
                    snprintf(g_real_ssids[g_real_count], 32, "%s", cleanSSID.c_str());
                    g_real_quality[g_real_count] = map(constrain(rssi, -100, -50), -100, -50, 0, 100);

                    Serial.printf("  💾 [快取 %d] %s (%d%%)\n", g_real_count + 1, g_real_ssids[g_real_count], g_real_quality[g_real_count]);
                    g_real_count++;
                }
            }

            WiFi.scanDelete(); // 清理暫存
            renderUI();        // 安全繪製 UI！
        }
    }

    // 🎯 3. 渲染絕對座標卡片
    void renderUI() {
        if (status_label) {
            lv_label_set_text(status_label, "[ SELECT WI-FI ]");
        }

        if (g_real_count == 0) {
            lv_obj_t * no_btn = lv_btn_create(lv_scr_act());
            lv_obj_set_size(no_btn, 280, 40);
            lv_obj_align(no_btn, LV_ALIGN_CENTER, 0, 0);
            
            lv_obj_t * lbl = lv_label_create(no_btn);
            lv_label_set_text(lbl, "No Wi-Fi Found");
            lv_obj_center(lbl);
            return;
        }

        // 使用絕對座標排列按鈕
        int startY = 40;
        for (int i = 0; i < g_real_count; i++) {
            lv_obj_t * btn = lv_btn_create(lv_scr_act());
            lv_obj_set_size(btn, 280, 32);
            lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, startY + (i * 38));
            
            // 亮眼配色：深藍底 + 紅色粗邊框
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x003366), 0);
            lv_obj_set_style_border_width(btn, 2, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xFF0000), 0);

            // 黃色文字
            String itemText = String(i + 1) + ". " + String(g_real_ssids[i]) + " (" + String(g_real_quality[i]) + "%)";
            
            lv_obj_t * label = lv_label_create(btn);
            lv_label_set_text(label, itemText.c_str());
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFF00), 0);
            lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
        }

        Serial.printf("🎨 [LVGL] 成功渲染 %d 個卡片！\n", g_real_count);
    }
};

#endif