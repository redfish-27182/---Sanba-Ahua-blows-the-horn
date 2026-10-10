#include <Arduino.h>
#include <WiFi.h>

#include "MqttTlsClient.h"
#include "secrets.h"

// MQTT 主題不放協定版本、日期或韌體版號，讓發布端與訂閱端長期維持不變。
constexpr char FIRMWARE_VERSION_TOPIC[] = "ahou/firmware_version";
constexpr char DEVICE_STATUS_PREFIX[] = "ahou/device_status/";
constexpr char DEVICE_STATUS_SUFFIX[] = "/status";

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000; // Wi-Fi 連線逾時時間，避免無限等待。
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;  //  

MqttTlsClient mqttClient;
String statusTopic;
uint32_t lastMqttAttemptAt = 0;

bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Connecting to Wi-Fi SSID: %s", WIFI_SSID);
    const uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi connection timed out; restarting shortly.");
        return false;
    }

    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

// 狀態主題使用單純字串，第一版僅回報 online 與 offline。
void publishStatus(const char *status, bool retained) {
    if (!mqttClient.publish(statusTopic.c_str(), status, retained)) {
        Serial.println("Unable to publish MQTT status.");
    }
}

// 韌體版本主題的 payload 就是版本字串，例如：1.0.1。
void onMqttMessage(char *topic, byte *payload, unsigned int length) {
    Serial.println("--- MQTT message received ---");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.write(payload, length);
    Serial.println();

    if (strcmp(topic, FIRMWARE_VERSION_TOPIC) == 0) {
        Serial.print("Available firmware version: ");
        Serial.write(payload, length);
        Serial.println();
    }
    Serial.println("-----------------------------");
}

// 連線到 MQTT Broker，並訂閱韌體版本主題。
void connectMqttIfDue() {
    if (mqttClient.connected() || millis() - lastMqttAttemptAt < MQTT_RETRY_INTERVAL_MS) {
        return;
    }
    lastMqttAttemptAt = millis();

    const String clientId = String(DEVICE_ID) + "-" + WiFi.macAddress();

    Serial.print("Connecting to MQTTS broker... ");
    if (!mqttClient.connect(
            clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD,
            statusTopic.c_str(), "offline")) {
        Serial.print("failed, PubSubClient state = ");
        Serial.println(mqttClient.state());
        return;
    }

    Serial.println("connected.");
    if (!mqttClient.subscribe(FIRMWARE_VERSION_TOPIC, 1)) {
        Serial.println("Subscription failed.");
        mqttClient.disconnect();
        return;
    }

    Serial.print("Subscribed to: ");
    Serial.println(FIRMWARE_VERSION_TOPIC);
    publishStatus("online", true);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nMQTT version receiver test starting.");

    statusTopic = String(DEVICE_STATUS_PREFIX) + DEVICE_ID + DEVICE_STATUS_SUFFIX;

    if (!connectWiFi() || !mqttClient.synchronizeClock()) {
        delay(5000);
        ESP.restart();
    }

    // TLS、根憑證與 MQTT buffer 都封裝在 MqttTlsClient 程式庫內。
    mqttClient.begin(MQTT_HOST, MQTT_PORT, onMqttMessage);
}

void loop() {
    // Wi-Fi 與 MQTT 重連各自處理；不使用阻塞式重連迴圈，
    // 未來才能與專案的 UI Task 一起運作。
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi disconnected; restarting to reconnect.");
        delay(1000);
        ESP.restart();
    }

    connectMqttIfDue(); // 連線到 MQTT Broker，並訂閱韌體版本主題。
    mqttClient.loop();
    delay(10);
}
