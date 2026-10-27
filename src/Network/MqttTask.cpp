#include "Network/MqttTask.h"

#include <WiFi.h>
#include <cstring>
#include <freertos/event_groups.h>

#include "App/AppMessages.h"
#include "App/AppQueues.h"
#include "Network/MqttTlsClient.h"
#include "Network/MqttTopics.h"
#include "Network/WifiScanner.h"
#include "secrets.h"

namespace {

MqttTlsClient mqttClient;
EventGroupHandle_t mqttStartupEvents = nullptr;
char weatherTopic[APP_TOPIC_LENGTH]{};
constexpr EventBits_t MQTT_INITIAL_SETUP_FINISHED = BIT0;
constexpr EventBits_t MQTT_BOOT_WIFI_SCAN_REQUESTED = BIT1;

// 連線失敗時依序等待 5、10、30、60 秒；不會因 Broker 暫時故障卡住整個系統。
constexpr uint32_t RECONNECT_DELAYS_MS[] = {5000, 10000, 30000, 60000};

// MQTT callback 會在 mqttClient.loop() 被呼叫時觸發，並且在同一個 Task 內執行。
void messageCallback(char *topic, byte *payload, unsigned int length) {
    // PubSubClient 的 payload 不是以 \0 結尾，因此必須明確複製並補結尾。
    // callback 在 MQTT Task 內被呼叫，只做無阻塞 Queue 轉送。
    if (topic == nullptr) {
        return;
    }

    // 天氣目前只供序列埠驗證，不解析、不存入 RAM，也不轉送其他 Task。
    if (strcmp(topic, weatherTopic) == 0) {
        Serial.printf("[Weather] topic=%s\n", topic);
        Serial.printf(
            "[Weather] payload=%.*s\n",
            static_cast<int>(length),
            reinterpret_cast<const char *>(payload)
        );
        return;
    }

    if (mqttInboundQueue == nullptr) {
        return;
    }

    MqttInboundEvent event{};
    strncpy(event.topic, topic, sizeof(event.topic) - 1);
    size_t copiedLength = static_cast<size_t>(length);
    if (copiedLength >= sizeof(event.payload)) {
        copiedLength = sizeof(event.payload) - 1;
    }
    memcpy(event.payload, payload, copiedLength);
    event.payload[copiedLength] = '\0';

    if (xQueueSend(mqttInboundQueue, &event, 0) != pdTRUE) {
        Serial.println("MQTT inbound queue full; message dropped.");
    }
}

// 嘗試連線到 MQTT Broker，並訂閱必要的主題。
bool connectAndSubscribe(char *statusTopic, size_t statusTopicLength) {
    snprintf(statusTopic, statusTopicLength, "%s%s%s", MQTT_DEVICE_STATUS_PREFIX,
            MQTT_DEVICE_ID, MQTT_DEVICE_STATUS_SUFFIX);
    snprintf(weatherTopic, sizeof(weatherTopic), "%s%s%s", MQTT_DEVICE_COMMAND_PREFIX,
            MQTT_DEVICE_ID, MQTT_DEVICE_WEATHER_SUFFIX);

    // Last Will 為 retained offline：裝置異常斷線時 Broker 仍會替我們留下離線狀態。
    if (!mqttClient.connect(MQTT_DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD, statusTopic, "offline")) {
        Serial.printf("MQTT connect failed, state=%d\n", mqttClient.state());
        return false;
    }

    // 訂閱裝置專屬韌體版本主題
    if (!mqttClient.subscribe(MQTT_FIRMWARE_VERSION_TOPIC, 1)) {
        Serial.println("MQTT 韌體版本訂閱失敗。");
        mqttClient.disconnect();
        return false;
    }

    // 訂閱裝置專屬的天氣主題
    if (!mqttClient.subscribe(weatherTopic, 1)) {
        Serial.println("MQTT 天氣訂閱失敗。");
        mqttClient.disconnect();
        return false;
    }

    mqttClient.publish(statusTopic, "online", true);
    Serial.println("MQTT 連接成功，並訂閱必要主題。");
    return true;
}

// 將 publish queue 的訊息發佈出去。
void drainPublishQueue() {
    MqttPublishRequest request{};
    while (xQueueReceive(mqttPublishQueue, &request, 0) == pdTRUE) {
        if (!mqttClient.publish(request.topic, request.payload, request.retained)) {
            Serial.println("MQTT 發布失敗了~ 哭哭");
        }
    }
}

// 開機掃描周遭環境 Wifi ，只發布一次
bool publishBootWifiScan() {
    if (!WifiScanner::hasResults()) {
        Serial.println("[WifiScanner] 沒有 Wi-Fi 掃描結果，無法發佈。");
        return true;
    }

    char topic[APP_TOPIC_LENGTH]{};
    char payload[384]{};
    snprintf(
        topic,
        sizeof(topic),
        "%s%s%s",
        MQTT_DEVICE_STATUS_PREFIX,
        MQTT_DEVICE_ID,
        MQTT_DEVICE_WIFI_SCAN_SUFFIX
    );
    if (!WifiScanner::writeLocationPayload(payload, sizeof(payload))) {
        Serial.println("[WifiScanner] 掃描結果轉換 JSON 失敗。");
        return true;
    }
    if (!mqttClient.publish(topic, payload, false)) {
        Serial.println("[WifiScanner] MQTT 發佈 Wi-Fi 掃描結果失敗。");
        return false;
    }
    Serial.printf("[WifiScanner] 發佈 Wi-Fi 掃描結果到 %s\n", topic);
    return true;
}

// MQTT Task 的主函式。
void mqttTask(void *) {
    mqttClient.begin(MQTT_HOST, MQTT_PORT, messageCallback);

    bool clockSynchronized = false;       // 是否已經同步過時間，避免 TLS 驗證失敗
    uint8_t retryIndex = 0;               // 連線失敗時的等待時間索引
    uint32_t nextAttemptAt = 0;           // 下一次嘗試連線的時間點（毫秒）
    char statusTopic[APP_TOPIC_LENGTH]{}; // 用來發佈 retained online/offline 狀態的主題
    bool bootWifiScanPublished = false;   // 開機 Wi-Fi 掃描結果是否已經發佈過
    bool bootWifiScanRequested = false;   // 其他 Task 是否要求發佈開機 Wi-Fi 掃描結果
    bool initialSetupPending = true;      // 是否尚未完成初始設定，讓其他 Task 可以等待

    for (;;) {

        // Wi-Fi 尚未連線，先等待 1 秒後再檢查。
        if (WiFi.status() != WL_CONNECTED) {
            if (mqttClient.connected()) {
                mqttClient.disconnect();
            }
            // Wi-Fi 重新連上後需要再次確認時間，避免斷線很久後 TLS 驗證失準。
            clockSynchronized = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 同步時間，避免 TLS 驗證失敗。
        if (!clockSynchronized) {
            clockSynchronized = mqttClient.synchronizeClock();
            if (!clockSynchronized) {
                if (initialSetupPending && mqttStartupEvents != nullptr) {
                    xEventGroupSetBits(mqttStartupEvents, MQTT_INITIAL_SETUP_FINISHED);
                    initialSetupPending = false;
                }
                vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAYS_MS[retryIndex]));
                retryIndex = retryIndex < 3 ? retryIndex + 1 : 3;
                continue;
            }
        }
        
        // MQTT 尚未連線，嘗試連線並訂閱主題。
        if (!mqttClient.connected()) {
            if (millis() >= nextAttemptAt) {
                if (connectAndSubscribe(statusTopic, sizeof(statusTopic))) {
                    retryIndex = 0;
                } else {
                    nextAttemptAt = millis() + RECONNECT_DELAYS_MS[retryIndex];
                    retryIndex = retryIndex < 3 ? retryIndex + 1 : 3;
                }
                if (initialSetupPending && mqttStartupEvents != nullptr) {
                    xEventGroupSetBits(mqttStartupEvents, MQTT_INITIAL_SETUP_FINISHED); // 讓其他 Task 可以等待初始設定完成
                    initialSetupPending = false;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // MQTT 已連線，檢查是否有其他 Task 要求發佈開機 Wi-Fi 掃描結果。
        if (mqttStartupEvents != nullptr &&
            (xEventGroupGetBits(mqttStartupEvents) & MQTT_BOOT_WIFI_SCAN_REQUESTED)) 
        {
            xEventGroupClearBits(mqttStartupEvents, MQTT_BOOT_WIFI_SCAN_REQUESTED);
            bootWifiScanRequested = true;
        }
        if (bootWifiScanRequested && !bootWifiScanPublished) {
            WifiScanner::scanAndLog();
            bootWifiScanPublished = publishBootWifiScan();
            bootWifiScanRequested = false;
        }

        drainPublishQueue(); // 將 publish queue 的訊息發佈出去。
        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

}  // namespace

// MqttTask 啟動!!!
void MqttTask_Start(UBaseType_t priority, BaseType_t core) {
    if (mqttStartupEvents == nullptr) {
        mqttStartupEvents = xEventGroupCreate(); // MQTT 初始設定完成事件
    }
    xTaskCreatePinnedToCore(
        mqttTask, 
        "MqttTask", 
        6144, 
        nullptr, 
        priority, 
        nullptr, 
        core
    );
}

// 等待 MqttTask 完成初始設定，或逾時。
bool MqttTask_WaitForInitialSetup(uint32_t timeoutMs) {
    if (mqttStartupEvents == nullptr) return false;
    return (xEventGroupWaitBits(
        mqttStartupEvents,
        MQTT_INITIAL_SETUP_FINISHED,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(timeoutMs)
    ) & MQTT_INITIAL_SETUP_FINISHED) != 0;
}

// 其他 Task 要求 MqttTask 發佈開機 Wi-Fi 掃描結果。
void MqttTask_RequestBootWifiScan() {
    if (mqttStartupEvents != nullptr) {
        // 設定事件旗標，MqttTask 會在下一個迴圈發佈 Wi-Fi 掃描結果。
        xEventGroupSetBits(mqttStartupEvents, MQTT_BOOT_WIFI_SCAN_REQUESTED);
    }
}
