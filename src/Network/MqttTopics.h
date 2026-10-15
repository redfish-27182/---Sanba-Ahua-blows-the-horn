#pragma once

// 後端廣播給所有 ESP32 的韌體版本指令。
constexpr char MQTT_FIRMWARE_VERSION_TOPIC[] =
    "ahou/commands/broadcast/firmware/version";

// ESP32 回傳事件時，一律放在自己的 device_id 節點下。
constexpr char MQTT_DEVICE_STATUS_PREFIX[] = "ahou/events/devices/";
constexpr char MQTT_DEVICE_STATUS_SUFFIX[] = "/status";

// 預留給 MQTT 連線與 Router 的測試訊息。
constexpr char MQTT_DEVICE_TEST_PING_SUFFIX[] = "/test/ping";
