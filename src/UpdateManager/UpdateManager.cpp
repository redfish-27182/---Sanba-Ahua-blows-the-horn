#include "UpdateManager/UpdateManager.h"

#include <cstring>

#include "App/AppMessages.h"
#include "App/AppQueues.h"
#include "Config.h"
#include "Network/MqttTopics.h"
#include "UpdateManager/GitHubOTA.h"

namespace {

    struct UpdateManagerContext {
        TFT_eSPI *tft;
    };

    // 告訴 UiTask 顯示訊息，並等待使用者回應。
    // type: 要顯示的訊息類型、 version: 韌體版本號、message: 顯示的訊息內容
    void sendUiCommand(
        UiCommandType type, 
        const char *version = "", 
        const char *message = ""
    ) {
        UiCommand command{};
        command.type = type;
        strncpy(command.version, version, sizeof(command.version) - 1);
        strncpy(command.message, message, sizeof(command.message) - 1);
        xQueueSend(uiCommandQueue, &command, pdMS_TO_TICKS(100)); // 傳送給 UiTask，等待 100ms 後放棄。
    }

    String buildFirmwareUrl(const char *version) {
        // MQTT 只傳版本字串；GitHub Release 的固定網址規則在這裡組合。
        return String(GITHUB_RELEASE_BASE_URL) + GITHUB_RELEASE_TAG_PREFIX + version + "/" + GITHUB_FIRMWARE_FILENAME;
    }

    // 等待使用者在 OTA 詢問框中做出決定，並回傳結果。
    bool waitForDecision(const char *version) {
        for (;;) {
            UiResponse response{}; // 從 UiTask 收到使用者的回應
            xQueueReceive(uiResponseQueue, &response, portMAX_DELAY); // 無限等待，直到收到回應
            if (strcmp(response.version, version) != 0) continue;
            if (response.type == UiResponseType::UPDATE_ACCEPTED) return true;
            if (response.type == UiResponseType::UPDATE_DECLINED) return false;
        }
    }

    // 等待 OTA 螢幕準備完成。
    void waitForOtaScreen(const char *version) {
        // GitHubOTA 會直接使用 TFT；先確認 UiTask 已停止一般畫面繪製。
        for (;;) {
            UiResponse response{};
            xQueueReceive(uiResponseQueue, &response, portMAX_DELAY);
            if (response.type == UiResponseType::OTA_SCREEN_READY &&
                strcmp(response.version, version) == 0) {
                return;
            }
        }
    }

    // UpdateManager Task 的主要邏輯：
    // 監聽 MQTT 訊息，詢問使用者是否更新，並執行 OTA。
    void updateManagerTask(void *parameter) {

        // 取得傳入的 TFT 物件，並釋放記憶體
        const UpdateManagerContext context = *static_cast<UpdateManagerContext *>(parameter);
        delete static_cast<UpdateManagerContext *>(parameter);
        GitHubOTA githubOta(*context.tft);

        // 持續監聽 MQTT 訊息
        for (;;) {
            // 接收 MQTT 訊息
            MqttInboundEvent event{}; //
            xQueueReceive(mqttInboundQueue, &event, portMAX_DELAY);

            // 這一段用來檢查MQTT傳過來的訊息是否是我們關心的韌體版本更新訊息
            if (strcmp(event.topic, MQTT_FIRMWARE_VERSION_TOPIC) != 0) {
                Serial.printf("Unhandled MQTT topic: %s\n", event.topic);
                continue;
            }

            // 這一段是用來檢查收到的版本號與目前的韌體版本是否相同，如果相同則不需要更新
            if (strcmp(event.payload, CURRENT_FIRMWARE_VERSION) == 0) {
                Serial.printf("Firmware already matches MQTT version: %s\n", event.payload);
                continue;
            }

            // 透過 UiTask 顯示更新提示，並等待使用者決定是否更新
            sendUiCommand(UiCommandType::SHOW_UPDATE_PROMPT, event.payload);
            if (!waitForDecision(event.payload)) {
                continue;
            }

            // 使用者同意更新，準備進行 OTA
            sendUiCommand(UiCommandType::PREPARE_FOR_OTA, event.payload);
            waitForOtaScreen(event.payload);

            // 建立 GitHub OTA 的下載網址，並開始 OTA 更新
            const String firmwareUrl = buildFirmwareUrl(event.payload);
            if (!githubOta.startOTA(firmwareUrl)) {
                sendUiCommand(UiCommandType::SHOW_NOTICE, event.payload, "OTA failed.");
                sendUiCommand(UiCommandType::RESUME_NORMAL_UI, event.payload);
                continue;
            }

            // 成功時 GitHubOTA 會重開機；此行是異常返回時的保護。
            sendUiCommand(UiCommandType::RESUME_NORMAL_UI, event.payload);
        }
    }

}  // namespace

// 啟動 UpdateManager Task，並傳入 TFT 物件以供 OTA 顯示進度。
void UpdateManager_Start(TFT_eSPI &tft, UBaseType_t priority, BaseType_t core) {
    auto *context = new UpdateManagerContext{&tft};
    xTaskCreatePinnedToCore(
        updateManagerTask, 
        "UpdateManager", 
        8192, 
        context, 
        priority, 
        nullptr, 
        core
    );
}
