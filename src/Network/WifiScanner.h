#pragma once

#include <stddef.h>
#include <stdint.h>

// 開機後掃描附近 Wi-Fi，並將結果輸出到序列埠。
class WifiScanner {
public:
    // 最大存取點數量，超過此數量的存取點將被忽略。
    static constexpr size_t maxAccessPoints = 5;

    // 掃描附近 Wi-Fi，並將結果輸出到序列埠。
    static void scanAndLog(); 

    // 掃描結果是否有足夠的定位資訊。
    static bool hasResults();

    // 將定位所需欄位序列化為 MQTT JSON；SSID 不會離開裝置。
    static bool writeLocationPayload(char *output, size_t outputSize);

private:
    struct AccessPoint {
        char bssid[18];
        int8_t rssi;
        uint8_t channel;
    };

    static AccessPoint accessPoints[maxAccessPoints];
    static size_t accessPointCount;
};
