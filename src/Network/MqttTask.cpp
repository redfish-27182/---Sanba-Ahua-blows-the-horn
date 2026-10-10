#include "Network/MqttTask.h"

#include <WiFi.h>
#include <cstring>

#include "App/AppMessages.h"
#include "App/AppQueues.h"
#include "Network/MqttTlsClient.h"
#include "Network/MqttTopics.h"
#include "Secrets.h"

namespace {

MqttTlsClient mqttClient;

// 連線失敗時依序等待 5、10、30、60 秒；不會因 Broker 暫時故障卡住整個系統。
constexpr uint32_t RECONNECT_DELAYS_MS[] = {5000, 10000, 30000, 60000};

// 連線成功後會立即發佈 retained online 狀態，斷線時 Broker 會自動發佈 retained offline 狀態。
void sendUiState(UiCommandType type) {
    if (uiCommandQueue == nullptr) {
        return;
    }

    UiCommand command{};
    command.type = type;
    xQueueSend(uiCommandQueue, &command, 0);
}

void messageCallback(char *topic, byte *payload, unsigned int length) {
    // PubSubClient 的 payload 不是以 \0 結尾，因此必須明確複製並補結尾。
    // callback 在 MQTT Task 內被呼叫，只做無阻塞 Queue 轉送。
    if (mqttInboundQueue == nullptr || topic == nullptr) {
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

bool connectAndSubscribe(char *statusTopic, size_t statusTopicLength) {
    snprintf(statusTopic, statusTopicLength, "%s%s%s", MQTT_DEVICE_STATUS_PREFIX,
            MQTT_DEVICE_ID, MQTT_DEVICE_STATUS_SUFFIX);

    // Last Will 為 retained offline：裝置異常斷線時 Broker 仍會替我們留下離線狀態。
    if (!mqttClient.connect(MQTT_DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD, statusTopic, "offline")) {
        Serial.printf("MQTT connect failed, state = %d\n", mqttClient.state());
        return false;
    }

    if (!mqttClient.subscribe(MQTT_FIRMWARE_VERSION_TOPIC, 1)) {
        Serial.println("MQTT subscribe failed.");
        mqttClient.disconnect();
        return false;
    }

    mqttClient.publish(statusTopic, "online", true);
    Serial.println("MQTT connected and subscribed.");
    sendUiState(UiCommandType::SET_MQTT_ONLINE);
    return true;
}

void drainPublishQueue() {
    MqttPublishRequest request{};
    while (xQueueReceive(mqttPublishQueue, &request, 0) == pdTRUE) {
        if (!mqttClient.publish(request.topic, request.payload, request.retained)) {
            Serial.println("MQTT publish failed; request dropped.");
        }
    }
}

void mqttTask(void *) {
    mqttClient.begin(MQTT_HOST, MQTT_PORT, messageCallback);

    bool clockSynchronized = false;
    bool wifiReportedOnline = false;
    uint8_t retryIndex = 0;
    uint32_t nextAttemptAt = 0;
    char statusTopic[APP_TOPIC_LENGTH]{};

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            if (mqttClient.connected()) {
                mqttClient.disconnect();
            }
            if (wifiReportedOnline) {
                sendUiState(UiCommandType::SET_WIFI_OFFLINE);
                sendUiState(UiCommandType::SET_MQTT_OFFLINE);
                wifiReportedOnline = false;
            }
            // Wi-Fi 重新連上後需要再次確認時間，避免斷線很久後 TLS 驗證失準。
            clockSynchronized = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (!wifiReportedOnline) {
            sendUiState(UiCommandType::SET_WIFI_ONLINE);
            wifiReportedOnline = true;
        }

        if (!clockSynchronized) {
            clockSynchronized = mqttClient.synchronizeClock();
            if (!clockSynchronized) {
                sendUiState(UiCommandType::SET_MQTT_OFFLINE);
                vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAYS_MS[retryIndex]));
                retryIndex = retryIndex < 3 ? retryIndex + 1 : 3;
                continue;
            }
        }

        if (!mqttClient.connected()) {
            if (millis() >= nextAttemptAt) {
                if (connectAndSubscribe(statusTopic, sizeof(statusTopic))) {
                    retryIndex = 0;
                } else {
                    sendUiState(UiCommandType::SET_MQTT_OFFLINE);
                    nextAttemptAt = millis() + RECONNECT_DELAYS_MS[retryIndex];
                    retryIndex = retryIndex < 3 ? retryIndex + 1 : 3;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        drainPublishQueue();
        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

}  // namespace

void MqttTask_Start(UBaseType_t priority, BaseType_t core) {
    xTaskCreatePinnedToCore(mqttTask, "MqttTask", 6144, nullptr, priority, nullptr, core);
}
