#include "UI/UiTask.h"

#include <cstdio>
#include <cstring>
#include <WiFi.h>

#include "App/AppMessages.h"
#include "App/AppQueues.h"

namespace {

    struct UiTaskContext {
        TFT_eSPI *tft;
        PromptDialog *dialog;
    };

    void sendResponse(UiResponseType type, const char *version = "") {
        UiResponse response{};
        response.type = type;
        strncpy(response.version, version, sizeof(response.version) - 1);
        xQueueSend(uiResponseQueue, &response, pdMS_TO_TICKS(100));
    }

    // 畫出一般畫面，顯示 WiFi 與 MQTT 狀態。
    void drawNormalScreen(TFT_eSPI &tft, bool wifiOnline, bool mqttOnline) {
        tft.fillScreen(TFT_BLACK);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("SanBa Ahou", 12, 12, 4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(wifiOnline ? "WiFi: online" : "WiFi: offline", 12, 70, 2);
        tft.drawString(mqttOnline ? "MQTT: online" : "MQTT: waiting", 12, 94, 2);
        tft.drawString("MQTT version update enabled", 12, 122, 2);
    }

    // 畫出動畫圓點，顯示 MQTT 連線狀態。
    void drawAnimationFrame(
        TFT_eSPI &tft, 
        uint16_t previousX, 
        uint16_t currentX,
        bool mqttOnline, 
        bool erasePrevious
    ) {
        // 只擦除上一個圓點再畫新圓點，避免整個螢幕被反覆清除而閃爍。
        if (erasePrevious) {
            tft.fillCircle(previousX, 190, 14, TFT_BLACK);
        }
        tft.fillCircle(currentX, 190, 12, mqttOnline ? TFT_GREEN : TFT_DARKGREY);
    }

    // 顯示 OTA 訊息是否更新成功或失敗的畫面，並停留三秒後回到一般畫面。
    void showNotice(TFT_eSPI &tft, const char *message) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("UPDATE NOTICE", 160, 62, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(message, 160, 105, 2);
    }

    // UiTask 的主要迴圈，負責處理 UI 命令與更新畫面。
    void uiTask(void *parameter) {

        // 取得 UiTaskContext，並釋放記憶體。
        const UiTaskContext context = *static_cast<UiTaskContext *>(parameter);
        delete static_cast<UiTaskContext *>(parameter);

        // 初始化 UI 狀態。
        bool normalUiEnabled = true; // 是否啟用一般畫面繪製
        bool wifiOnline = WiFi.status() == WL_CONNECTED;
        bool mqttOnline = false; // MQTT 連線狀態由 CoordinatorTask 更新
        uint16_t animationX = 20; // 動畫圓點的 X 座標
        uint16_t previousAnimationX = animationX;
        int8_t animationDirection = 1;
        uint32_t lastFrameAt = 0; // 上一個畫面更新的時間戳記
        bool needsNormalRedraw = true; // 是否需要重新繪製一般畫面

        for (;;) {
            UiCommand command{};
            if (xQueueReceive(uiCommandQueue, &command, pdMS_TO_TICKS(30)) == pdTRUE) {
                switch (command.type) {
                    case UiCommandType::SHOW_UPDATE_PROMPT: {
                        // PromptDialog 會讀取既有 inputQueue；它只能由 UiTask 呼叫，避免 TFT 競爭。
                        String message = "New firmware: ";
                        message += command.version;
                        message += "\nUpdate now?";
                        const bool accepted = context.dialog->show("SYSTEM UPDATE", message,
                                                                    "[A] UPDATE", "[B] LATER");
                        sendResponse(accepted ? UiResponseType::UPDATE_ACCEPTED
                                                : UiResponseType::UPDATE_DECLINED,
                                    command.version);
                        needsNormalRedraw = true;
                        break;
                    }
                    case UiCommandType::SHOW_NOTICE:
                        showNotice(*context.tft, command.message);
                        // OTA 失敗畫面停留三秒，但只阻塞 UiTask；MQTT 與背景工作仍會執行。
                        vTaskDelay(pdMS_TO_TICKS(3000));
                        needsNormalRedraw = true;
                        break;
                    case UiCommandType::PREPARE_FOR_OTA:
                        // GitHubOTA 是既有且已驗證的程式，會直接操作 TFT。
                        // 先停止自己的繪圖並回覆確認，之後才允許 Coordinator 啟動 OTA。
                        normalUiEnabled = false;
                        context.tft->fillScreen(TFT_BLACK);
                        sendResponse(UiResponseType::OTA_SCREEN_READY, command.version);
                        break;
                    case UiCommandType::RESUME_NORMAL_UI:
                        normalUiEnabled = true;
                        needsNormalRedraw = true;
                        break;
                    case UiCommandType::SET_WIFI_ONLINE:
                        wifiOnline = true;
                        needsNormalRedraw = true;
                        break;
                    case UiCommandType::SET_WIFI_OFFLINE:
                        wifiOnline = false;
                        mqttOnline = false;
                        needsNormalRedraw = true;
                        break;
                    case UiCommandType::SET_MQTT_ONLINE:
                        mqttOnline = true;
                        needsNormalRedraw = true;
                        break;
                    case UiCommandType::SET_MQTT_OFFLINE:
                        mqttOnline = false;
                        needsNormalRedraw = true;
                        break;
                }
            }
            // 每隔 80ms 更新一次畫面，避免過度消耗 CPU。
            if (normalUiEnabled && millis() - lastFrameAt >= 80) {
                if (needsNormalRedraw) {
                    drawNormalScreen(*context.tft, wifiOnline, mqttOnline);
                    drawAnimationFrame(*context.tft, animationX, animationX, mqttOnline, false);
                    needsNormalRedraw = false;
                } else {
                    drawAnimationFrame(*context.tft, previousAnimationX, animationX, mqttOnline, true);
                }
                previousAnimationX = animationX;
                animationX = static_cast<uint16_t>(static_cast<int>(animationX) + animationDirection * 4);
                if (animationX >= 300 || animationX <= 20) {
                    animationDirection = -animationDirection;
                }
                lastFrameAt = millis();
            }
        }
    }
} 


// UiTask，啟動!!!
void UiTask_Start(TFT_eSPI &tft, PromptDialog &dialog, UBaseType_t priority, BaseType_t core) {
    auto *context = new UiTaskContext{&tft, &dialog};
    xTaskCreatePinnedToCore(uiTask, "UiTask", 4096, context, priority, nullptr, core);
}
