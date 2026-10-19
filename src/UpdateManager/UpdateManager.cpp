#include "UpdateManager/UpdateManager.h"
#include <cstring>
#include "Config.h"
#include "Network/MqttTopics.h"
#include "UpdateManager/GitHubOTA.h"

// 處理韌體版本並在使用者同意後同步執行更新。
// event：const MqttInboundEvent&，MQTT 訊息；tft：TFT_eSPI&，螢幕；dialog：PromptDialog&，提示框。
// 回傳：bool，是否顯示過提示或更新畫面。
bool UpdateManager_Handle(const MqttInboundEvent &event, TFT_eSPI &tft, PromptDialog &dialog) {
    if (strcmp(event.topic, MQTT_FIRMWARE_VERSION_TOPIC) != 0) {
        Serial.printf("Unhandled MQTT topic: %s\n", event.topic);
        return false;
    }
    if (strcmp(event.payload, CURRENT_FIRMWARE_VERSION) == 0) return false;
    String message = "New firmware: ";
    message += event.payload;
    message += "\nUpdate now?";
    if (!dialog.show("SYSTEM UPDATE", message, "[A] UPDATE", "[B] LATER")) return true;
    const String url = String(GITHUB_RELEASE_BASE_URL) + GITHUB_RELEASE_TAG_PREFIX + event.payload + "/" + GITHUB_FIRMWARE_FILENAME;
    GitHubOTA ota(tft);
    if (!ota.startOTA(url)) {
        delay(3000);
        Serial.println("OTA failed; returning to game.");
    }
    return true;
}
