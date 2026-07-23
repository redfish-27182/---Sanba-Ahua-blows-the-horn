#ifndef FASTAPI_CLIENT_H
#define FASTAPI_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 用來包裹伺服器回傳結果的小盒子 (Struct)
struct UpdateInfo {
    bool has_update;
    String latest_version;
    String download_url;
    String message;
};

class FastAPIClient {
private:
    String baseUrl;

public:
    // 建構子：傳入你的 FastAPI 網址
    FastAPIClient(String serverUrl) {
        baseUrl = serverUrl;
    }

    // 查詢更新
    bool checkUpdate(String currentVersion, UpdateInfo &info) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("❌ Wi-Fi 未連線");
            return false;
        }

        HTTPClient http;
        String fullUrl = baseUrl + "/check_update?current_version=" + currentVersion;

        Serial.print("🔍 正在向 FastAPI 查詢更新: ");
        Serial.println(fullUrl);

        http.begin(fullUrl);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                // 將伺服器資料填入 info
                info.has_update = doc["has_update"] | false;
                info.latest_version = doc["latest_version"] | "";
                info.download_url = doc["download_url"] | "";
                info.message = doc["message"] | "";

                http.end();
                return true;
            } else {
                Serial.print("❌ JSON 解析失敗: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.printf("❌ HTTP 請求失敗，ErrorCode: %d\n", httpCode);
        }

        http.end();
        return false;
    }
};

#endif