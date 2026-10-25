#include "Network/WifiScanner.h"

#include <WiFi.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace {

// 將 wifi_auth_mode_t 轉換為可讀的字串。
const char *securityName(wifi_auth_mode_t security) {
    switch (security) {
        case WIFI_AUTH_OPEN: return "open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "unknown";
    }
}

// 將格式化字串附加到輸出緩衝區，並更新已使用的字元數。
bool appendToPayload(char *output, size_t outputSize, size_t &used, const char *format, ...) {
    if (used >= outputSize) return false;

    va_list arguments; 
    va_start(arguments, format);
    const int written = vsnprintf(output + used, outputSize - used, format, arguments);
    va_end(arguments);
    if (written < 0 || static_cast<size_t>(written) >= outputSize - used) return false;
    used += static_cast<size_t>(written);
    return true;
}

}  // namespace

// WifiScanner 類別的靜態成員初始化
WifiScanner::AccessPoint WifiScanner::accessPoints[WifiScanner::maxAccessPoints]{};

// 目前已儲存的存取點數量。
size_t WifiScanner::accessPointCount = 0;

// 同步掃描附近的存取點。此階段只供人工檢查，尚未傳送到 MQTT。
void WifiScanner::scanAndLog() {
    Serial.println("[WifiScanner] 掃描附近 wifi 中...");
    accessPointCount = 0;
    const int networkCount = WiFi.scanNetworks(); 

    if (networkCount < 0) {
        Serial.printf("[WifiScanner] 掃描失敗, status=%d\n", networkCount);
        WiFi.scanDelete();
        return;
    }

    Serial.printf("[WifiScanner] 發現 %d 個 Wi-Fi 網路\n", networkCount);
    for (int index = 0; index < networkCount; ++index) {
        Serial.printf(
            "[WifiScanner] #%d SSID=%s | BSSID=%s | RSSI=%d dBm | channel=%d | security=%s\n",
            index + 1,
            WiFi.SSID(index).c_str(),
            WiFi.BSSIDstr(index).c_str(),
            WiFi.RSSI(index),
            WiFi.channel(index),
            securityName(WiFi.encryptionType(index))
        );

        // WiFi.scanNetworks() 的結果依訊號強度排序，只保留定位足夠的前五筆。
        if (accessPointCount < maxAccessPoints) {
            const String bssid = WiFi.BSSIDstr(index);
            strncpy(
                accessPoints[accessPointCount].bssid, 
                bssid.c_str(), 
                sizeof(accessPoints[accessPointCount].bssid) - 1
            );
            accessPoints[accessPointCount].rssi = static_cast<int8_t>(WiFi.RSSI(index));
            accessPoints[accessPointCount].channel = static_cast<uint8_t>(WiFi.channel(index));
            ++accessPointCount;
        }
    }

    // 釋放 WiFi.scanNetworks() 建立的掃描結果記憶體。
    WiFi.scanDelete();
}

bool WifiScanner::hasResults() {
    return accessPointCount > 0;
}

// 將定位所需欄位序列化為 MQTT JSON；SSID 不會離開裝置。
bool WifiScanner::writeLocationPayload(char *output, size_t outputSize) {
    // 檢查輸入參數是否有效，並確認有掃描結果。
    if (output == nullptr || outputSize == 0 || !hasResults()) {
        return false;
    }
    Serial.println("[WifiScanner] 序列化 Wi-Fi 掃描結果...");

    size_t used = 0;
    if (!appendToPayload(output, outputSize, used, "{\"access_points\":[")) {
        return false;
    }
    for (size_t index = 0; index < accessPointCount; ++index) {
        if (!appendToPayload(
                output, outputSize, used,
                "%s{\"bssid\":\"%s\",\"rssi\":%d,\"channel\":%u}",
                index == 0 ? "" : ",",
                accessPoints[index].bssid,
                accessPoints[index].rssi,
                accessPoints[index].channel
            )) return false;
    }
    return appendToPayload(output, outputSize, used, "]}");
}
