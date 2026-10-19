# Tiled 地圖轉檔工具

兩個工具分別處理 `.tmx` 地圖與 `.png` 瓦片圖片。它們只讀取 `input/` 中的素材，**不開啟 `.tsx`**，也不修改原始檔或直接讀取專案 `map/` 資料夾。

## 放入檔案

將地圖 `.tmx` 和畫這張地圖所用的瓦片 PNG 複製到 `input/`，可以使用子資料夾：

```text
input/
├── first_map.tmx
└── spr_tileset_sunnysideworld.png
```

不需要複製 `.tsx`。工具不讀取 `.tsx`，無法自動確認你放的 PNG 是不是 Tiled 繪圖時所用的圖片，**請自行確認 PNG 正確且瓦片排列沒有改動**。

`input/` 和 `output/` 的內容預設不加入 Git，請自行備份素材及輸出。

## 安裝（Windows PowerShell）

需要 Python 3.9 以上。在終端機進入此資料夾，第一次使用時建立環境並安裝 Pillow：

```powershell
cd tools/map_converter
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

若 `py -3` 找不到 Python，請改用已安裝的 Python 3 執行檔完整路徑，執行 `-m venv .venv`。安裝完成後不必每次重複安裝。

## 第一次轉檔

在 `tools/map_converter` 目錄執行：

```powershell
.\.venv\Scripts\python.exe map_tool.py scan
.\.venv\Scripts\python.exe tileset_tool.py
.\.venv\Scripts\python.exe map_tool.py export
```

1. `map_tool.py scan` 讀取 `.tmx`，產生本次地圖需要的瓦片編號清單。
2. `tileset_tool.py` 讀取 PNG 和清單，產生精簡瓦片庫。它不讀取 `.tmx`。
3. `map_tool.py export` 讀取 `.tmx` 和已建立的瓦片庫，輸出地圖資料。它不讀取 PNG。

如果 `input/` 內不只有一張 PNG，第二步要指定相對於 `input/` 的路徑：

```powershell
.\.venv\Scripts\python.exe tileset_tool.py --png "spr_tileset_sunnysideworld.png"
```

工具按固定規則處理：16 × 16、沒有 margin 或 spacing、先由左到右再由上到下。PNG 的寬高需要是 16 的倍數。若地圖圖層或群組設定了像素偏移，工具會提示，**輸出時統一按 `0, 0` 處理，不更動來源地圖**。

## 輸出檔案

`output/` 會包含：

- `tileset.bin`：精簡原始瓦片庫，包含排序後的原始瓦片 ID 和 RGB565 像素。
- `<地圖名稱>.bin`：地圖尺寸、圖層和每格的瓦片編號與翻轉旗標。
- `tile_requirements.json`、`tileset.index.json`：電腦端工具的中間清單及對照資料，不需要放進 SD 卡。

同名檔案會覆寫，舊輸出不會自動刪除。轉檔失敗時，`output/` 可能留有上次的舊檔；請以本次成功輸出的訊息為準。

這是**格式版本 2**，與舊版預先產生翻轉圖片的轉檔結果不相容；請重新執行上面三步，不要混用舊檔。ESP32 端的讀取和繪圖程式尚未實作，現在還無法直接在 ESP32 上顯示。

## 修改或新增地圖

如果地圖只改了排列或翻轉，使用的原始瓦片都已在現有瓦片庫中，可以只執行：

```powershell
.\.venv\Scripts\python.exe map_tool.py scan
.\.venv\Scripts\python.exe map_tool.py export
```

這不會重建 `tileset.bin`。地圖檔會包含目前素材庫的識別碼；修改翻轉也不會增加素材庫中的圖片。

如果新增了素材庫沒有的瓦片，地圖工具會提示缺少瓦片。此時請將**所有需要共用這個素材庫的地圖**放入 `input/`，再重新執行第一次轉檔的三步。重建後所有地圖檔和瓦片庫應一起更新到 SD 卡。若更改 PNG 本身，也要重新建庫並重新輸出全部對應地圖；光是重新掃描 `.tmx` 不會檢查 PNG 是否改變。

## 目前限制

- 一張有限大小的正交地圖只能有一個瓦片集；所有地圖需使用同一個瓦片集。地圖工具只檢查 `.tmx` 中的瓦片集名稱，不開啟 `.tsx`。
- 瓦片圖層使用未壓縮 CSV，可放在群組中。物件圖層、圖片圖層、隱藏圖層、半透明圖層、色調、視差及非零格子座標偏移目前不支援，會報錯。
- 不支援需要 `.tsx` 才能知道的瓦片設定，例如 margin、spacing、瓦片動畫或自訂繪圖偏移，請勿在來源素材使用。
- PNG 像素以 alpha `128` 為門檻：`0～127` 轉為 RGB565 `0xF81F` 透明色鍵，`128～255` 使用原本顏色並視為不透明。工具會顯示本次使用的瓦片中處理了多少半透明像素，不修改原始 PNG；半透明邊緣可能變硬。若不透明像素轉成 RGB565 後撞到色鍵，工具會停止並提示。
- PNG 最多 8191 片瓦片：地圖的 13 bits 使用 `0` 表示空格，`1～8191` 分別對應 PNG 原始瓦片 ID `0～8190`。
- 翻轉資訊保存在地圖檔，素材庫不另外存翻轉副本。日後 ESP32 需要依 Tiled 規則先處理對角翻轉，再水平、垂直翻轉。
- 輸出大小不是 SRAM 足夠的保證；需要在 ESP32 同時啟動 Wi-Fi、MQTT 等功能後實測。

## 二進位格式版本 2

標頭、原始瓦片 ID 表和地圖格子都用 **little-endian**；RGB565 每個像素使用**高位元組在前**。ESP32 端需要配合顯示器傳送方式驗證色彩與位元組順序。

### `tileset.bin`

| 欄位 | 型別 | 說明 |
| --- | --- | --- |
| magic | 4 bytes | `AHTS` |
| version | uint16 | `2` |
| tile size | uint16 | `16` |
| count | uint32 | 實際載入的瓦片數 |
| library ID | 16 bytes | ID 表與像素資料的 SHA-256 前 16 bytes |
| tile IDs | uint16 × count | 排序後的原始 PNG 瓦片 ID，從 0 開始 |
| pixels | 512 bytes × count | 與 ID 表同順序的 RGB565 原始瓦片 |

ESP32 可在排序後的 ID 表中搜尋原始瓦片 ID，再用該位置找到對應的 512 bytes；一片瓦片只需 2 bytes 對照資料。例如 137 種瓦片需 274 bytes 的 ID 表。

### `<地圖名稱>.bin`

| 欄位 | 型別 | 說明 |
| --- | --- | --- |
| magic | 4 bytes | `AHMP` |
| version | uint16 | `2` |
| width / height / layers | uint16 × 3 | 地圖寬、高及圖層數 |
| library ID | 16 bytes | 必須與 `tileset.bin` 的識別碼一致 |
| cells | uint16[] | 依圖層順序，再由上到下、由左到右 |

每個 cell 的低 13 bits 儲存 `原始瓦片 ID + 1`；`0` 是空格。bit 13 是水平翻轉、bit 14 是垂直翻轉、bit 15 是對角翻轉。這個編碼不同於 Tiled 原始的 32-bit GID；地圖工具會在轉檔時處理 `firstgid` 與旗標。

## 自動測試

測試只建立臨時合成素材，不讀取使用者的地圖：

```powershell
.\.venv\Scripts\python.exe -m unittest -v test_tools
```
