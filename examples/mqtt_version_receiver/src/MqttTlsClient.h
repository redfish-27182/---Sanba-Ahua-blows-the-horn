#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// 封裝 MQTTS 所需的根憑證、TLS client、NTP 校時與 PubSubClient。
// 使用端不需要接觸憑證內容，也不應呼叫 setInsecure() 略過驗證。
class MqttTlsClient {
public:
    using MessageCallback = void (*)(char *topic, byte *payload, unsigned int length);

    void begin(const char *host, uint16_t port, MessageCallback callback);
    bool synchronizeClock(uint32_t timeoutMs = 15000);

    bool connect(
        const char *clientId,
        const char *username,
        const char *password,
        const char *willTopic,
        const char *willPayload);
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
