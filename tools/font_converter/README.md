# 16×16 字庫轉換工具

將字型與 UTF-8 字元清單轉成單色點陣字庫。ESP32 開機時載入系統字庫以顯示中文提示；信件功能尚未實作。

## 1. 下載字型

1. 開啟 [Noto Sans CJK TC Regular 官方檔案頁](https://github.com/notofonts/noto-cjk/blob/main/Sans/OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf)。
2. 點右上角的 **Download raw file** 下載，檔名為 `NotoSansCJKtc-Regular.otf`。
3. 把檔案放到 `tools/font_converter/input/`。

字型由 Noto 專案以 SIL Open Font License 發布；若散布字型或轉換後字庫，請查看並遵循[官方授權條款](https://github.com/notofonts/noto-cjk/blob/main/Sans/LICENSE)。本專案沒有附帶字型檔。

## 2. 安裝工具

在 `tools/font_converter/` 的 PowerShell 執行：

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

如果 `py -3` 找不到 Python，請使用本機 Python 3 的完整路徑執行 `-m venv .venv`。

## 3. 轉換字庫

```powershell
.\.venv\Scripts\python.exe font_tool.py system
.\.venv\Scripts\python.exe font_tool.py common
```

輸出為 `output/system.bin` 與 `output/common.bin`；成功時會覆寫同名輸出檔。輸入和輸出資料夾不會加入 Git。

- `characters/system.txt`：系統字元清單，可自行修改。
- `characters/common.txt`：約 400 個中文字與標點的試用清單，**不保證收錄所有常用字**，可隨時編輯。
- 工具會去重、排序，自動加入 ASCII 可列印字元及缺字標記 `□`。所以最終總字數會多於中文字數。
- 每字居中繪製於 16×16，再以亮度門檻 128 轉為單色；小字筆畫與邊界需在實機檢查。
- 字型本身若缺少清單字元，工具會報錯，不會把錯誤字形輸出成有效字。ESP32 遇到字庫未收錄的字會顯示 `□`。

## ESP32 使用方式

將重新產生的 `output/system.bin` 複製到 SD 卡的 `/fonts/system.bin`，再上傳韌體。開機時會載入字庫，Wi-Fi、對話框及其他系統提示即可顯示中文；載入失敗時使用英文備用文字。`common.bin` 暫時不用放入 SD 卡，信件畫面尚未實作。

可指定其他字型或清單：

```powershell
.\.venv\Scripts\python.exe font_tool.py common --font "input/你的字型.otf" --chars "characters/common.txt"
```

## 二進位格式 v1

所有整數皆 little-endian。標頭共 16 bytes：`AHFT`（4 bytes）、版本（uint16，1）、寬度（uint16，16）、高度（uint16，16）、保留欄位（uint16，0）、字數（uint32）。

後接依 Unicode 編號遞增的紀錄，每筆 36 bytes：Unicode 編號（uint32）及 32 bytes 點陣。點陣逐行、每行兩個 bytes，從左至右依最高位到最低位排列；1 為有墨水，0 為透明背景。

## 測試

```powershell
.\.venv\Scripts\python.exe -m unittest -v test_font_tool
```
