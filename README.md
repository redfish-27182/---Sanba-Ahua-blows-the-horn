# 三八阿花吹喇叭 

> **增進人與人之間感情上的連結的神奇物聯網互動裝置**

---

## 專案簡介

「三八阿花吹喇叭」是一個將實體硬體與電玩世界結合的互動專案。

* **無線空中升級**：支援無縫遠端韌體更新，並具備動態像素風進度條。
* **復古遊戲視界**：搭載 `TFT_eSPI` 繪製 320x240 懷舊點陣畫面。
* **虛實地圖串聯**：包含十字路口、大海木棧道信箱與許願池等場景設計。

---

## 硬體與技術

### 硬體
* **主控板**：ESP32 (ESP32-2432S028 / 黃色便宜小板板)
* **顯示器**：2.8 吋 TFT 液晶螢幕 (320x240, SPI 介面)
* **扭動的阿花**
  
<table border="10">
  <tr>
    <td width="50%" align="center">
      <img src="https://github.com/redfish-27182/---Sanba-Ahua-blows-the-horn/blob/main/sunton_esp32_2432S028.jpg" width="100%">
      <br>
      <sub><b>ESP32 黃色便宜小板板</b></sub>
    </td>
    <td width="50%" align="center">
      <img src="https://github.com/redfish-27182/---Sanba-Ahua-blows-the-horn/blob/main/%E9%98%BF%E8%8A%B1%E5%9C%96%E7%89%87.jpg" width="100%">
      <br>
      <sub><b>阿花</b></sub>
    </td>
  </tr>
</table>

### 軟體 & 程式庫
* **開發環境**：VS Code + PlatformIO (ESP32) + Python (server)
* **核心庫**：
  * `TFT_eSPI` - 螢幕驅動與圖形繪製
  * `HTTPUpdate.h` - HTTP OTA 線上升級
  * `WiFi.h` - 無線網路連線管理
  * `FreeRTOS.h` - 多任務管理架構
  * `FastAPI` - 雲端資料管理 / HTTP請求

---

## 目前專案結構

```text
.
├── platformio.ini                       # PlatformIO 設定與程式庫相依
└── src/
    ├── main.cpp                         # 硬體、Wi‑Fi、與 Task 啟動
    ├── Config.h                         # 腳位、目前版本與更新網址
    ├── EventTypes.h                     # 觸控事件 InputEvent 與 inputQueue 宣告
    ├── User_Setup.h                     # TFT_eSPI 螢幕設定
    ├── App/
    │   ├── AppMessages.h                # Task 之間傳遞的訊息結構
    │   └── AppQueues.h / .cpp           # Queue 宣告、實體與建立函式
    ├── Network/
    │   ├── MqttTlsClient.h / .cpp       # TLS CA 憑證、校時( EMQX需要 )
    │   ├── MqttTopics.h                 # MQTT 的 Topic 路徑
    │   └── MqttTask.h / .cpp            # MQTT 連線、訂閱、收發與重連 Task
    ├── Tasks/
    │   └── TouchTask.h / .cpp           # 產生觸控／手勢事件
    ├── UI/
    │   ├── PromptDialog.h / .cpp        # 觸控確認對話框
    │   └── UiTask.h / .cpp              # 渲染畫面
    └── UpdateManager/
        ├── UpdateManager.h / .cpp       # 版本比較與通知更新
        ├── GitHubOTA.h                  # OTA 更新 
        └── ImageDownloader.h            # 舊的圖片下載模組 (之後需要完善)
```

## FreeRTOS Task 分工

| Task | 核心 | 優先權 | Stack | 責任 |
| --- | ---: | ---: | ---: | --- |
| `TouchTask` | Core 1 | 4 | 4096 bytes | 讀取觸控、判斷 TAP／DRAG／LONG_PRESS／SWIPE，寫入 `inputQueue`。 |
| `UiTask` | Core 1 | 3 | 4096 bytes | 正常狀態下唯一持續使用 TFT 的 Task；繪製簡單動畫、顯示網路狀態、呼叫 `PromptDialog`。 |
| `UpdateManager` | Core 0 | 2 | 8192 bytes | 接收 MQTT 版本訊息，詢問使用者、組 OTA URL，並呼叫 `GitHubOTA`。 |
| `MqttTask` | Core 0 | 2 | 6144 bytes | MQTTS 校時、連線、訂閱、重連、訊息收發與裝置狀態發布。 |


## Queue 與資料流

跨 Task 的資料使用固定大小的 struct 放入 Queue，避免共用 `String` 或直接跨 Task 操作物件。

| Queue | 生產者 | 消費者 | 資料型別 | 用途 |
| --- | --- | --- | --- | --- |
| `inputQueue` | `TouchTask` | `PromptDialog`（由 `UiTask` 呼叫） | `InputEvent` | 觸控與手勢事件。 |
| `mqttInboundQueue` | `MqttTask` | `UpdateManager` | `MqttInboundEvent` | MQTT 收到的主題與 payload。 |
| `mqttPublishQueue` | 未來其他 Task | `MqttTask` | `MqttPublishRequest` | 要由 MQTT 背景 Task 發送的訊息；目前保留給後續功能。 |
| `uiCommandQueue` | `MqttTask`、`UpdateManager` | `UiTask` | `UiCommand` | 網路狀態、更新詢問、OTA 前暫停繪圖與通知。 |
| `uiResponseQueue` | `UiTask` | `UpdateManager` | `UiResponse` | 使用者確認／取消，以及 TFT 已可供 OTA 使用的確認。 |

```text
TouchTask ── inputQueue ──> PromptDialog（UiTask）

MqttTask ── mqttInboundQueue ──> UpdateManager
                                      │
                                      ├── uiCommandQueue ──> UiTask / PromptDialog
                                      └<─ uiResponseQueue ── UiTask
                                      │
                                      └── GitHubOTA ──> GitHub Release firmware.bin

其他 Task ── mqttPublishQueue ──> MqttTask ──> EMQX Broker
MqttTask ── uiCommandQueue ──> UiTask（Wi‑Fi／MQTT 狀態）
```

## ⚠️ 開發注意事項

* **觸控晶片與螢幕走線不同**：
  小黃板 (ESP32-2432S028) 的 XPT2046 觸控晶片與螢幕、SD 卡使用不同的實體引腳（觸控走線為固定腳位：`CLK: 25`, `MISO: 39`, `MOSI: 32`, `CS: 33`）。

  **解法**：不能使用 `TFT_eSPI` 的內建觸控 API（如 `tft.getTouch()`），必須使用獨立的 `XPT2046_Touchscreen` 函式庫。


* **螢幕與 SD 卡 SPI 通訊配置衝突**：
  ESP32 內部具備兩套獨立的 SPI 控制器（`VSPI` 與 `HSPI`）。螢幕與 SD 卡預設使用 `VSPI`（引腳 18, 19, 23）；若將觸控也配置在 `VSPI`，呼叫 `SD.begin()` 時會覆寫觸控腳位映射，導致觸控通訊中斷（讀取值卡在 `4095` 或無回應）。

  **解法**：將觸控物件綁定至獨立的 **`HSPI`** 控制器，這樣的話實體線路完全無需變動即可讓 SD 卡讀寫與觸控同時運作。
