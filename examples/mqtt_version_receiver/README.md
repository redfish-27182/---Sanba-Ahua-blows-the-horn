# MQTT version receiver test

這是一個獨立的 ESP32 / PlatformIO 手動測試專案，不放在根目錄的 `test/`。根目錄的 `test/` 是 PlatformIO Test Runner（Unity 單元測試）慣例目錄；本範例需要持續連線至 Wi-Fi 與 MQTT broker，因此使用可單獨編譯、上傳的 PlatformIO 專案。

## 功能

- 透過 MQTTS（TLS，port 8883）連線 EMQX Serverless。
- 訂閱 retained topic：`ahou/firmware_version`。
- 在序列埠印出純文字版本字串，例如 `1.0.1`；不使用 JSON。
- 發佈 retained 狀態至 `ahou/device_status/esp32-mqtt-version-test/status`；成功連線時為 `online`，非正常斷線時 MQTT LWT 會改為 `offline`。
- TLS 根憑證與 NTP 校時已封裝至 `lib/MqttTlsClient`，主程式不需處理憑證內容。

## 憑證與設定

本機已建立 `include/secrets.h`，且 `.gitignore` 已排除它。若要在另一台電腦使用，請將 `include/secrets.example.h` 複製成 `include/secrets.h`，再填入自己的 Wi-Fi、MQTT 帳密與裝置 ID。

EMQX Serverless 僅支援 TLS 連線；範例保留 CA 驗證，並在開始 MQTT 連線前以 NTP 校時，不能以 `setInsecure()` 取代。

## 編譯、上傳與監看

在專案根目錄執行：

```powershell
pio run -d examples/mqtt_version_receiver -t upload
pio device monitor -d examples/mqtt_version_receiver
```

序列埠速率為 115200。

## 從 MQTTX 發佈測試版本

在 MQTTX 對 `ahou/firmware_version` 發佈以下純文字，並開啟 **Retain**。ESP32 訂閱後應立刻在 Serial Monitor 顯示 `Available firmware version`：

```text
1.0.1
```

若 TLS 連線失敗，請先確認 EMQX Cloud Overview 中的 deployment 狀態、網域、帳密與 CA 憑證；Serverless 必須使用完整網域和 port 8883。
