#include "UpdateManager/UpdateManager.h"
#include <cstring>
#include "Config.h"
#include "UpdateManager/GitHubOTA.h"
#include "Font/SystemText.h"

// 處理韌體版本並在使用者同意後同步執行更新。
// update：const FirmwareUpdate&，版本資料；tft：TFT_eSPI&，螢幕；dialog：PromptDialog&，提示框。
// 回傳：bool，是否顯示過提示或更新畫面。
bool UpdateManager_Handle(const FirmwareUpdate &update, TFT_eSPI &tft, PromptDialog &dialog) {
    if (strcmp(update.version, CURRENT_FIRMWARE_VERSION) == 0) return false;
    String message = SystemText_Choose("新韌體：", "New firmware: ");
    message += update.version;
    message += SystemText_Choose("\n現在更新嗎？", "\nUpdate now?");
    if (!dialog.show(SystemText_Choose("系統更新", "SYSTEM UPDATE"), message, SystemText_Choose("更新", "UPDATE"), SystemText_Choose("稍後", "LATER"))) return true;
    const String url = String(GITHUB_RELEASE_BASE_URL) + GITHUB_RELEASE_TAG_PREFIX + update.version + "/" + GITHUB_FIRMWARE_FILENAME;
    GitHubOTA ota(tft);
    if (!ota.startOTA(url)) {
        delay(3000);
        Serial.println("OTA failed; returning to game.");
    }
    return true;
}
