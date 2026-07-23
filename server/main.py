from fastapi import FastAPI, Query
from pydantic import BaseModel

app = FastAPI(title="Sanba Ahua OTA Server")

# 🎯 設定 GitHub 專案的固定資訊
GITHUB_USER = "redfish-27182"
REPO_NAME = "---Sanba-Ahua-blows-the-horn"

# 🎯 目前伺服器上的最新版本號與預設檔名
LATEST_VERSION = "v0.1.0-ota-test"
FIRMWARE_FILENAME = "update_test.bin"

class UpdateResponse(BaseModel):
    has_update: bool
    latest_version: str
    download_url: str
    message: str

def build_github_release_url(tag: str, filename: str) -> str:
    """自動拼裝出 GitHub Release 的直鏈下載網址"""
    return f"https://github.com/{GITHUB_USER}/{REPO_NAME}/releases/download/{tag}/{filename}"

@app.get("/check_update", response_model=UpdateResponse)
def check_update(current_version: str = Query(..., description="ESP32 當前韌體版本")):
    print(f"📡 收到 ESP32 查詢要求！目前板子版本: {current_version}")
    
    # 比對版本號
    if current_version != LATEST_VERSION:
        # 🎯 動態合成 GitHub 下載網址
        download_url = build_github_release_url(LATEST_VERSION, FIRMWARE_FILENAME)
        
        return UpdateResponse(
            has_update=True,
            latest_version=LATEST_VERSION,
            download_url=download_url,
            message=f"發現新版本 {LATEST_VERSION}！準備開始 OTA 升級。"
        )
    else:
        return UpdateResponse(
            has_update=False,
            latest_version=LATEST_VERSION,
            download_url="",
            message="目前已是最新版本，無需更新。"
        )

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)