#pragma once

// 主題刻意不放協定版本、韌體版本或日期，讓訂閱規則長期穩定。
constexpr char MQTT_FIRMWARE_VERSION_TOPIC[] = "ahou/firmware_version";
constexpr char MQTT_DEVICE_STATUS_PREFIX[] = "ahou/device_status/";
constexpr char MQTT_DEVICE_STATUS_SUFFIX[] = "/status";
