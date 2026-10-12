# SanBa_Ahou 後端

這是 ESP32 的 MQTT 狀態接收與韌體版本廣播後端。HTTP 資料夾已先保留，第一版尚未實作網站 API。

## 第一次設定

本機需要 **Python 3.11 以上**。目前系統的 `python` 指令若仍指向 Python 2.7，請安裝 Python 3 後以 `py -3` 執行以下指令。

在 `server` 資料夾開啟 PowerShell：

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

`.env` 不會被 Git 追蹤。請在其中填入：

- `DATABASE_PASSWORD`：剛建立的 `sanba_app` 資料庫帳號密碼。
- `MQTT_HOST`、`MQTT_USERNAME`、`MQTT_PASSWORD`：輪替後的 EMQX 設定。

EMQX Cloud 使用公開 CA 時，`MQTT_CA_CERT_PATH` 可保留空白，Python 會使用系統信任的 CA 驗證 TLS。

## 建立資料表

每次資料庫結構更新都由 Alembic 管理。第一次先執行：

```powershell
.\.venv\Scripts\python.exe -m alembic upgrade head
```

這會建立 `device_status` 資料表，用來存每台 ESP32 最後一次上線或離線的狀態。

## 啟動後端

```powershell
.\.venv\Scripts\python.exe -m uvicorn app.main:app --reload
```

啟動後，後端會：

1. 以 TLS 連線到 EMQX。
2. 訂閱 `ahou/device_status/+/status`。
3. 收到 `online` 或 `offline` 時，更新 PostgreSQL。

`ahou/firmware_version` 是 retained 的全域版本公告 topic。現在先提供程式內的發布函式；等網站 HTTP API 開始實作時，再由網站功能呼叫它。
