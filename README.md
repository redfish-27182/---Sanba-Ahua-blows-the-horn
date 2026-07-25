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
  * `HTTPUpdate.h` & `HTTPClient.h` - HTTP OTA 線上升級
  * `WiFi.h` - 無線網路連線管理
  * `FastAPI` - 雲端資料管理 / HTTP請求

---

## 最新功能亮點

### HTTP OTA 無線升級與動態進度條
* 支援透過區域網路 / 雲端 HTTP 伺服器下載 `.bin` 韌體。
* **即時下載回呼 (Progress Callback)**：下載過程中螢幕不凍結，即時顯示下載百分比與像素風格綠色進度條。

## ⚠️ 開發注意事項

小黃板 (CYD) 的 XPT2046 觸控晶片與 ILI9341 螢幕**沒有共用 SPI 匯流排**（觸控為獨立腳位：`CLK: 25`, `MISO: 39`, `MOSI: 32`, `CS: 33`）。

因此**不能使用 `TFT_eSPI` 的內建觸控 API**（如 `tft.getTouch()`），否則會完全沒有反應。

**解決方案**：必須使用獨立的 [`XPT2046_Touchscreen`](https://github.com/PaulStoffregen/XPT2046_Touchscreen) 程式庫
