import os
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.responses import JSONResponse
import uvicorn

app = FastAPI(title="ESP32 Asset & Version Server")

# -------------------------------------------------------------------
# ⚙️ 基本設定 (請依照你的環境修改)
# -------------------------------------------------------------------
SERVER_HOST = "http://192.168.0.101:8000"

LATEST_VERSION = "0.1.1"

# GitHub 的 bin 下載網址完全保持你剛才貼的那串即可：
GITHUB_FIRMWARE_URL = "https://github.com/redfish-27182/---Sanba-Ahua-blows-the-horn/releases/download/v0.1.1/firmware.bin"

# -------------------------------------------------------------------
# 📁 自動建立圖片目錄 & 靜態檔案掛載
# -------------------------------------------------------------------
STATIC_DIR = "static"
IMAGES_DIR = os.path.join(STATIC_DIR, f"images_v{LATEST_VERSION}")

# 若圖片資料夾不存在則自動建立，避免開啟伺服器時報錯
os.makedirs(IMAGES_DIR, exist_ok=True)

# 掛載 static 資料夾，讓外界可以直接用 URL (GET) 下載圖片檔案
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


# -------------------------------------------------------------------
# 📡 ESP32 查詢配置的 API
# -------------------------------------------------------------------
@app.get("/api/v1/config")
def get_system_config():
    """
    ESP32 開機時會 GET 這個 API 獲取配置 JSON
    """
    image_urls = []

    # 自動掃描 static/images_v1.0.1/ 資料夾裡面的圖片檔
    if os.path.exists(IMAGES_DIR):
        for filename in sorted(os.listdir(IMAGES_DIR)):
            if filename.lower().endswith(('.bmp', '.png', '.jpg', '.jpeg')):
                img_url = f"{SERVER_HOST}/static/images_v{LATEST_VERSION}/{filename}"
                image_urls.append(img_url)

    # 回傳極簡 JSON 給 ESP32
    return JSONResponse(content={
        "firmware_version": LATEST_VERSION,
        "firmware_url": GITHUB_FIRMWARE_URL, # 傳回 GitHub 網址給 ESP32 的 GitHubOTA
        "images": image_urls                 # 傳回本地 Server 的圖片下載網址清單
    })


# -------------------------------------------------------------------
# 🚀 啟動伺服器
# -------------------------------------------------------------------
if __name__ == "__main__":
    print(f"🚀 ESP32 更新伺服器啟動中...")
    print(f"📌 當前版本號: {LATEST_VERSION}")
    print(f"📁 圖片存放目錄: {os.path.abspath(IMAGES_DIR)}")
    print(f"🔗 GitHub OTA 網址: {GITHUB_FIRMWARE_URL}")
    print(f"🌐 本地 API 網址: {SERVER_HOST}/api/v1/config")
    
    # host="0.0.0.0" 才能讓區網內的 ESP32 成功連線
    uvicorn.run(app, host="0.0.0.0", port=8000)