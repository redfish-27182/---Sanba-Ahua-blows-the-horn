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

void sendUiCommand(UiCommandType type, const char *version = "", const char *message = "") {
    UiCommand command{};
    command.type = type;
    strncpy(command.version, version, sizeof(command.version) - 1);
    strncpy(command.message, message, sizeof(command.message) - 1);
    xQueueSend(uiCommandQueue, &command, pdMS_TO_TICKS(100));
}

String buildFirmwareUrl(const char *version) {
    // MQTT 只傳版本字串；GitHub Release 的固定網址規則在這裡組合。
    return String(GITHUB_RELEASE_BASE_URL) + GITHUB_RELEASE_TAG_PREFIX + version + "/" +
           GITHUB_FIRMWARE_FILENAME;
}

bool waitForDecision(const char *version) {
    // PromptDialog 本身會等待觸控；UpdateManager 只等待 UiTask 回傳選擇。
    for (;;) {
        UiResponse response{};
        xQueueReceive(uiResponseQueue, &response, portMAX_DELAY);

        if (strcmp(response.version, version) != 0) {
            continue;
        }
        if (response.type == UiResponseType::UPDATE_ACCEPTED) {
            return true;
        }
        if (response.type == UiResponseType::UPDATE_DECLINED) {
            return false;
        }
    }
}

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

void updateManagerTask(void *parameter) {
    const UpdateManagerContext context = *static_cast<UpdateManagerContext *>(parameter);
    delete static_cast<UpdateManagerContext *>(parameter);
    GitHubOTA githubOta(*context.tft);

    for (;;) {
        MqttInboundEvent event{};
        xQueueReceive(mqttInboundQueue, &event, portMAX_DELAY);

        if (strcmp(event.topic, MQTT_FIRMWARE_VERSION_TOPIC) != 0) {
            // 未來新增 MQTT 參數時，可在這裡依主題分派給各自的處理器。
            Serial.printf("Unhandled MQTT topic: %s\n", event.topic);
            continue;
        }

        // 發布流程保證版本字串正確，因此不做格式驗證、數字比較或檔案存在檢查。
        // 只要版本字串和目前韌體不同，就允許使用者進行更新或降版。
        if (strcmp(event.payload, CURRENT_FIRMWARE_VERSION) == 0) {
            Serial.printf("Firmware already matches MQTT version: %s\n", event.payload);
            continue;
        }

        sendUiCommand(UiCommandType::SHOW_UPDATE_PROMPT, event.payload);
        if (!waitForDecision(event.payload)) {
            // 不記錄取消結果；Broker 再發布同一版本時，仍會再次詢問。
            continue;
        }

        sendUiCommand(UiCommandType::PREPARE_FOR_OTA, event.payload);
        waitForOtaScreen(event.payload);

        const String firmwareUrl = buildFirmwareUrl(event.payload);
        if (!githubOta.startOTA(firmwareUrl)) {
            // GitHubOTA 已顯示錯誤畫面；再保留三秒提示後回到一般 UI。
            // 不重試、不記錄失敗；下次 MQTT 訊息來時可重新嘗試。
            sendUiCommand(UiCommandType::SHOW_NOTICE, event.payload, "OTA failed.");
            sendUiCommand(UiCommandType::RESUME_NORMAL_UI, event.payload);
            continue;
        }

        // 成功時 GitHubOTA 會重開機；此行是異常返回時的保護。
        sendUiCommand(UiCommandType::RESUME_NORMAL_UI, event.payload);
    }
}

}  // namespace

void UpdateManager_Start(TFT_eSPI &tft, UBaseType_t priority, BaseType_t core) {
    auto *context = new UpdateManagerContext{&tft};
    xTaskCreatePinnedToCore(updateManagerTask, "UpdateManager", 8192, context, priority, nullptr, core);
}
