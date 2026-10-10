#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// 將 TLS 憑證、NTP 校時與 PubSubClient 集中在這裡。
// 上層 Task 不需要知道憑證細節，也不應該使用 setInsecure()。
class MqttTlsClient {
public:
    using MessageCallback = void (*)(char *topic, byte *payload, unsigned int length);

    void begin(const char *host, uint16_t port, MessageCallback callback);
    bool synchronizeClock(uint32_t timeoutMs = 15000);
    bool connect(const char *clientId, const char *username, const char *password,
                 const char *willTopic, const char *willPayload);
    bool subscribe(const char *topic, uint8_t qos);
    bool publish(const char *topic, const char *payload, bool retained);
    bool connected();
    void disconnect();
    void loop();
    int state();

private:
    WiFiClientSecure secureClient;
    PubSubClient mqttClient{secureClient};
};
