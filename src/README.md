# UiTask 架構

本文件記錄目前的 UI、地圖與韌體更新流程。圖中的 `SceneManager`、`GameScene` 和 `UpdateManager` 是由 `UiTask` 呼叫的模組，**不是各自獨立的 FreeRTOS Task**。

## 整體架構

```text
TouchTask                               MQTT Task
    │                                       │
    │ 觸控事件                              ├── 收到 MQTT 訊息
    ▼                                       │       ▼
inputQueue                                  │  mqttInboundQueue
    │                                       │       │
    └──────────────────────┬────────────────────────┘
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ UiTask                                                       │
│                                                              │
│ 接收事件與訊息，安排遊戲畫面、提示視窗和 OTA 更新。          │
│                                                              │
│  ├── SceneManager                                            │
│  │     ├── LoadingScene → DisplayDriver → TFT_eSPI           │
│  │     ├── MapLoader → SD 卡 → MapData（SRAM）                │
│  │     └── GameScene                                         │
│  │           ├── Camera：鏡頭原點與移動範圍                  │
│  │           └── TileRenderer                                │
│  │                 ├── 讀取 MapData 和 Camera                │
│  │                 └── DisplayDriver → TFT_eSPI              │
│  │                                                           │
│  └── UpdateManager                                           │
│        ├── PromptDialog → TFT_eSPI                           │
│        └── GitHubOTA → TFT_eSPI                              │
└──────────────────────────────────────────────────────────────┘
```

目前 `UiTask` 和 `TouchTask` 固定在 Core 1；`MqttTask` 固定在 Core 0。遊戲、提示視窗與 OTA 依序使用螢幕，不同時繪製。開機初始化階段則由 `main.cpp` 使用螢幕和既有的 Wi-Fi 提示視窗。

## 檔案職責

| 檔案 | 職責 |
| --- | --- |
| `UI/UiTask.cpp` | 接收佇列事件，呼叫畫面管理與更新流程。 |
| `UI/SceneManager.cpp` | 顯示載入畫面、呼叫地圖載入器、管理遊戲畫面及重畫。 |
| `UI/Scenes/LoadingScene.h` | 顯示載入中或錯誤訊息。 |
| `UI/Scenes/GameScene.cpp` | 接收觸控事件，更新鏡頭並安排遊戲畫面重繪。 |
| `UI/PromptDialog.cpp` | 繪製提示視窗，直接從 `inputQueue` 等待使用者回答。 |
| `Game/Camera.cpp` | 管理鏡頭原點，根據拖曳位移移動並限制邊界。 |
| `Map/MapData.h` | 定義 SRAM 中的地圖、瓦片索引與像素資料。 |
| `Map/MapLoader.cpp` | 從 SD 卡載入並檢查地圖和瓦片素材。 |
| `Display/TileRenderer.cpp` | 根據鏡頭和地圖拼裝可見圖層，處理透明色與翻轉。 |
| `Display/DisplayDriver.cpp` | 透過 TFT_eSPI 顯示狀態文字或像素區塊。 |
| `UpdateManager/UpdateManager.cpp` | 判斷韌體版本、詢問使用者，並呼叫既有 OTA。 |
| `UpdateManager/GitHubOTA.h` | 下載和寫入韌體，並直接繪製 OTA 進度。 |

## 開機與遊戲流程

```text
開機初始化 → UiTask → SceneManager → LoadingScene
                                         │
                                         ▼
                                    MapLoader
                                         │
                         ┌───────────────┴───────────────┐
                         ▼                               ▼
                       成功                            失敗
                         │                               │
                         ▼                               ▼
                    建立 GameScene                  顯示錯誤
                         │
                         ▼
           TouchTask → inputQueue → UiTask → GameScene
                                                 │
                                                 ▼
                                 Camera → TileRenderer → 螢幕
```

`DRAG` 事件提供目前座標及按下時的起點；`GameScene` 計算相鄰事件的位移，再更新 `Camera`。鏡頭初始原點為 `(0, 0)`，以像素為單位。畫面需要更新時，渲染器使用小型緩衝區同步繪製可見區域；遊戲渲染期間不讀取 SD 卡。

## 韌體更新流程

```text
MQTT Task → mqttInboundQueue → UiTask → UpdateManager
                                             │
                                             ▼
                                       PromptDialog
                                             │
                               ┌─────────────┴─────────────┐
                               ▼                           ▼
                             拒絕                         同意
                               │                           │
                               ▼                           ▼
                           重畫遊戲                   GitHubOTA
                                                    顯示更新進度
                                                          │
                                              ┌───────────┴──────────┐
                                              ▼                      ▼
                                            失敗                    成功
                                              │                      │
                                              ▼                      ▼
                                         恢復並重畫遊戲             重啟
```

提示視窗等待回答及 OTA 執行期間，`UiTask` 不會繼續更新或渲染遊戲，也不會處理一般 UI 指令。其他 Task 仍會執行，訊息可能在等待期間累積；佇列滿時可能丟失訊息。提示或更新結束後，`UiTask` 清除累積的觸控事件，再要求畫面重畫。

## 佇列分工

| 佇列 | 用途 |
| --- | --- |
| `inputQueue` | `TouchTask` 傳送觸控事件；一般遊戲時由 `UiTask` 讀取，提示視窗開啟時由 `PromptDialog` 讀取。 |
| `mqttInboundQueue` | MQTT Task 傳送收到的訊息，由 `UiTask` 交給更新邏輯。 |
| `mqttPublishQueue` | 傳送發布請求給 MQTT Task。 |

`UpdateManager` 不再是獨立 Task，因此目前不需要 `uiResponseQueue` 來交接使用者選擇或 OTA 螢幕使用權。

## SD 卡檔案

```text
SD 卡根目錄 /
└── game/
    ├── maps/
    │   └── first_map.bin
    └── tilesets/
        └── tileset.bin
```

這兩個檔案需使用相容的轉檔結果。開機載入後，地圖與所需瓦片素材保存在 SRAM；原始 `.tmx`、`.png` 及轉檔用 `.json` 不必放到 SD 卡。
