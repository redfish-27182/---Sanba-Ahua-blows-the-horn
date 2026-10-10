#include "Network/MqttTlsClient.h"

namespace {

// EMQX Cloud Serverless 使用的 DigiCert Global Root G2。
// 憑證只集中在此檔，日後要改成自己的程式庫時可整個搬走。
constexpr char ROOT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

}  // namespace

void MqttTlsClient::begin(const char *host, uint16_t port, MessageCallback callback) {
    secureClient.setCACert(ROOT_CA);
    mqttClient.setServer(host, port);
    mqttClient.setCallback(callback);
    mqttClient.setKeepAlive(30);
    mqttClient.setBufferSize(512);
}

bool MqttTlsClient::synchronizeClock(uint32_t timeoutMs) {
    // TLS 驗證需要正確時間；校時失敗時絕不退回不驗證憑證的連線方式。
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    tm timeInfo{};
    Serial.print("Waiting for NTP time");
    const uint32_t startedAt = millis();
    while (!getLocalTime(&timeInfo) && millis() - startedAt < timeoutMs) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();

    if (!getLocalTime(&timeInfo)) {
        Serial.println("NTP failed; TLS certificate validation cannot continue.");
        return false;
    }

    Serial.println("System time synchronized.");
    return true;
}

bool MqttTlsClient::connect(const char *clientId, const char *username, const char *password,
                            const char *willTopic, const char *willPayload) {
    return mqttClient.connect(clientId, username, password, willTopic, 1, true, willPayload);
}

bool MqttTlsClient::subscribe(const char *topic, uint8_t qos) { return mqttClient.subscribe(topic, qos); }
bool MqttTlsClient::publish(const char *topic, const char *payload, bool retained) {
    return mqttClient.publish(topic, payload, retained);
}
bool MqttTlsClient::connected() { return mqttClient.connected(); }
void MqttTlsClient::disconnect() { mqttClient.disconnect(); }
void MqttTlsClient::loop() { mqttClient.loop(); }
int MqttTlsClient::state() { return mqttClient.state(); }
