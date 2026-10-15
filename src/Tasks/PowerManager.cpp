#include "PowerManager.h"

#include <esp_sleep.h>

#include "../Config.h"

namespace {

volatile uint32_t lastActivityMs = 0;
volatile bool displaySleeping = false;

void setDisplayAwake(bool awake) {
    // 本專案的背光腳位為 HIGH 亮、LOW 關。
    digitalWrite(TFT_BL, awake ? HIGH : LOW);
    displaySleeping = !awake;
}

bool isTouchIrqReleased() {
    // XPT2046 的 IRQ 在未觸碰時通常為 HIGH；觸碰時為 LOW。
    // 進入 deep sleep 前必須確認已放開，避免立刻被同一次觸碰喚醒。
    return digitalRead(XPT2046_IRQ) == HIGH;
}

void enterDeepSleep() {
    if (!isTouchIrqReleased()) {
        return;
    }

    Serial.println("Entering deep sleep. Touch the screen to wake.");

    // GPIO36 是 RTC GPIO，可用 XPT2046 的低電位 IRQ 作為喚醒來源。
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(XPT2046_IRQ), LOW);
    Serial.flush();
    esp_deep_sleep_start();
}

void powerManagerTask(void *pvParameters) {
    for (;;) {
        const uint32_t idleMs = millis() - lastActivityMs;

        if (idleMs >= DEEP_SLEEP_TIMEOUT_MS) {
            enterDeepSleep();
        } else if (idleMs >= DISPLAY_SLEEP_TIMEOUT_MS && !displaySleeping) {
            Serial.println("Display sleep: backlight off.");
            setDisplayAwake(false);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}  // namespace

void PowerManager_Start(UBaseType_t priority, BaseType_t coreID) {
    // GPIO36 沒有內建上拉電阻；觸控模組的 IRQ 腳位必須已由硬體上拉。
    pinMode(XPT2046_IRQ, INPUT);
    lastActivityMs = millis();
    setDisplayAwake(true);

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
        Serial.println("Display wake: the current touch is ignored.");
        setDisplayAwake(true);
    }

    return wasDisplaySleeping;
}
