#include <esp_sleep.h>

#include "PowerManager.h"
#include "../Config.h"

namespace {

volatile uint32_t lastActivityMs = 0;  // 最後一次觸控活動的時間戳記
volatile bool displaySleeping = false; // 目前螢幕背光是否已關閉

// 關閉或開啟螢幕背光，並更新 displaySleeping 狀態。
void setDisplayAwake(bool awake) {
    digitalWrite(TFT_BL, awake ? HIGH : LOW);
    displaySleeping = !awake;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
}

// 進入深度休眠，並設定觸控 IRQ 腳位為喚醒來源。
void enterDeepSleep() {
    if (!digitalRead(XPT2046_IRQ) == HIGH) {
        return;
    }

    Serial.println("[PowerManager] 進入深度睡眠，等待觸控喚醒。");

    // GPIO36 是 RTC GPIO，可用 XPT2046 的低電位 IRQ 作為喚醒來源。
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(XPT2046_IRQ), LOW);
    Serial.flush(); // 確保序列埠輸出完成，避免進入深度睡眠後訊息無法顯示。
    esp_deep_sleep_start();
}

// FreeRTOS 工作本體：檢查閒置時間，必要時關閉背光或進入深度休眠。
void powerManagerTask(void *pvParameters) {
    for (;;) {
        // 計算目前的閒置時間。
        const uint32_t idleMs = millis() - lastActivityMs;

        if (idleMs >= DEEP_SLEEP_TIMEOUT_MS) {
            enterDeepSleep();
        } else if (idleMs >= DISPLAY_SLEEP_TIMEOUT_MS && !displaySleeping) {
            Serial.println("[PowerManager] 螢幕閒置超過 20 秒，關閉背光。");
            setDisplayAwake(false);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}  // namespace

// freeRTOS 工作建立函式
void PowerManager_Start(UBaseType_t priority, BaseType_t coreID) {
    // GPIO36 沒有內建上拉電阻；觸控模組的 IRQ 腳位必須已由硬體上拉。
    pinMode(XPT2046_IRQ, INPUT);
    lastActivityMs = millis(); // 初始化最後活動時間
    setDisplayAwake(true); // 確保啟動時背光開啟

    xTaskCreatePinnedToCore(
        powerManagerTask,
        "PowerManager",
        3072,
        nullptr,
        priority,
        nullptr,
        coreID
    );
}

// 觸控中斷服務程式中呼叫此函式，更新最後活動時間。
bool PowerManager_HandleTouchActivity() {
    const bool wasDisplaySleeping = displaySleeping;
    lastActivityMs = millis();

    if (wasDisplaySleeping) {
        Serial.println("[PowerManager] 觸控活動，喚醒背光。");
        setDisplayAwake(true);
    }

    return wasDisplaySleeping;
}
