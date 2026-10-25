#include "Network/WifiScanner.h"

#include <WiFi.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace {

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

WifiScanner::AccessPoint WifiScanner::accessPoints[WifiScanner::maxAccessPoints]{};
size_t WifiScanner::accessPointCount = 0;

// 同步掃描附近的存取點。此階段只供人工檢查，尚未傳送到 MQTT。
void WifiScanner::scanAndLog() {
    Serial.println("[WifiScanner] Scanning nearby Wi-Fi networks...");
    accessPointCount = 0;
    const int networkCount = WiFi.scanNetworks();

    if (networkCount < 0) {
        Serial.printf("[WifiScanner] Scan failed, status=%d\n", networkCount);
        WiFi.scanDelete();
        return;
    }

    Serial.printf("[WifiScanner] Found %d network(s)\n", networkCount);
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
            strncpy(accessPoints[accessPointCount].bssid, bssid.c_str(), sizeof(accessPoints[accessPointCount].bssid) - 1);
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

bool WifiScanner::writeLocationPayload(char *output, size_t outputSize) {
    if (output == nullptr || outputSize == 0 || !hasResults()) return false;

    size_t used = 0;
    if (!appendToPayload(output, outputSize, used, "{\"access_points\":[")) return false;
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
