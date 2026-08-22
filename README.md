# 三八阿花吹喇叭 

> **一個結合 2D 像素風 RPG 遊戲、雲端 OTA 升級 與 天氣 API 查詢 的神奇物聯網互動裝置。**

---

## 專案簡介

「三八阿花吹喇叭」是一個將實體硬體與電玩世界結合的互動專案。透過 主控 ESP32 的黃色便宜小板板，將遠端的訊息、動態天氣與互動事件，化為懷舊像素風的遊戲場景。

* **無線空中升級 (HTTP OTA)**：支援無縫遠端韌體更新，並具備動態像素風進度條。
* **復古遊戲視界**：搭載 `TFT_eSPI` 繪製 320x240 懷舊點陣畫面。
* **虛實地圖串聯**：包含十字路口、大海木棧道信箱與許願池等場景設計。

---

## 硬體與技術棧

### 硬體
* **主控板**：ESP32 (ESP32-2432S028 / 黃色便宜小板板)
* **顯示器**：2.8 吋 TFT 液晶螢幕 (320x240, SPI 介面)
  
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

## 專案目錄與架構 

本專案基於 **ESP32 FreeRTOS 多任務架構** 進行模組化設計。系統將「觸控採樣」、「畫面渲染」、「聯網更新」與「UI 對話框」各模組解耦，透過 FreeRTOS Queue 進行跨任務資料傳遞。

```text
.
├── platformio.ini              # PlatformIO 專案配置檔與相依庫管理
└── src/
    ├── main.cpp                # 系統主入口：硬體初始化、WiFi 配網、OTA 檢查與 Task 啟動
    ├── Config.h                # 全域硬體腳位定義與系統逾時設定
    ├── EventTypes.h            # FreeRTOS 隊列資料結構與全域 Queue 宣告
    ├── User_Setup.h            # TFT_eSPI 驅動設定檔
    │
    ├── Tasks/                  # 🧵 FreeRTOS 任務執行層 (獨立執行緒)
    │   ├── TouchTask.h / .cpp  # 觸控任務：獨立 HSPI 雙重採樣、抗跳點濾波與手勢結算
    │   └── DisplayTask.h / .cpp# 渲染任務：接收 Queue 事件、1 秒影子回放渲染與 Light-Sleep 睡眠管理
    │
    ├── UI/                     # 🎨 介面與彈窗元件層
    │   └── PromptDialog.h / .cpp # 阻塞式/互動式彈窗元件 (支援觸控按鈕判定)
    │
    └── UpdateManager/          # 🌐 網路與系統更新模組
        ├── UpdateManager.h / .cpp # OTA 版本比對與更新流程調度
        ├── GitHubOTA.h         # 韌體下載與 Flash 燒錄核心
        ├── ImageDownloader.h   # SD 卡靜態資源/圖片更新器
        └── README.md           # OTA 模組說明文件
```

## 最新功能亮點

### HTTP OTA 無線升級與動態進度條
* 支援透過區域網路 / 雲端 HTTP 伺服器下載 `.bin` 韌體。
* **即時下載回呼 (Progress Callback)**：下載過程中螢幕不凍結，即時顯示下載百分比與像素風格綠色進度條。

## ⚠️ 開發注意事項

小黃板 (CYD / ESP32-2432S028) 的 XPT2046 觸控晶片與螢幕、SD 卡使用不同的實體引腳（觸控走線為固定腳位：`CLK: 25`, `MISO: 39`, `MOSI: 32`, `CS: 33`）。不能使用 `TFT_eSPI` 的內建觸控 API（如 `tft.getTouch()`），必須使用獨立的 `XPT2046_Touchscreen` 函式庫。

* **使用獨立 HSPI 硬體引擎（解決 SD 卡 / SPI 衝突）**：
  ESP32 內部具備兩套獨立的 SPI 控制器（`VSPI` 與 `HSPI`）。螢幕與 SD 卡預設使用 `VSPI`（引腳 18, 19, 23）；若將觸控也配置在 `VSPI`，呼叫 `SD.begin()` 時會覆寫觸控腳位映射，導致觸控通訊中斷（讀取值卡在 `4095` 或無回應）。
  
  **最佳解法**：利用 ESP32 的 **GPIO Matrix（引腳矩陣）** 特性，將觸控物件綁定至獨立的 **`HSPI`** 控制器，實體線路完全無需變動即可讓 SD 卡讀寫與觸控常駐監聽（如休眠計時）並行運作：
