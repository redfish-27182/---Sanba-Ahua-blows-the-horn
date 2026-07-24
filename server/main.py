import os
import socket
from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse

app = FastAPI(title="ESP32 OTA & Asset Auto-Scanner Server")

# ==================== ⚙️ 系統設定區 ====================
# 1. 韌體設定
CURRENT_FW_VER = "1.0.1"
FIRMWARE_URL   = "https://github.com/redfish-27182/---Sanba-Ahua-blows-the-horn/releases/download/v0.1.1/firmware.bin"

# 2. 圖片資產版本設定 (修改這個數字，伺服器就會自動去對應的資料夾抓圖)
CURRENT_ASSET_VER = 101  # 對應資料夾 static/images/v101/

# 3. 伺服器網路設定
SERVER_PORT = 8000
# =======================================================


# 取得本機在區域網路 (Wi-Fi) 中的真實 IP 位址
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "192.168.0.100"  # 若抓取失敗則使用預設備用 IP

SERVER_IP = get_local_ip()
BASE_IMAGE_DIR = os.path.join(os.path.dirname(__file__), "static", "images")


# 🎯 API 1: 提供系統配置，自動掃描資料夾並生成 URL 清單
@app.get("/api/v1/config")
def get_system_config():
    """
    ESP32 開機請求此介面，自動回傳當前版本與資料夾下的所有圖片下載 URL
    """
    version_dir_name = f"v{CURRENT_ASSET_VER}"
    target_dir = os.path.join(BASE_IMAGE_DIR, version_dir_name)
    
    image_urls = []
    
    # 檢查該版本的圖片資料夾是否存在
    if os.path.exists(target_dir) and os.path.isdir(target_dir):
        # 掃描資料夾內的所有檔案
        for filename in os.listdir(target_dir):
            # 只抓取圖片格式檔案 (可依需求擴充)
            if filename.lower().endswith(('.bmp', '.png', '.jpg', '.jpeg')):
                # 動態拼湊出 ESP32 可讀取的完整下載 URL
                url = f"http://{SERVER_IP}:{SERVER_PORT}/api/v1/download/image/{version_dir_name}/{filename}"
                image_urls.append(url)
        
        # 依照檔名排序，確保每次下載順序一致
        image_urls.sort()

    return {
        "firmware_version": CURRENT_FW_VER,
        "firmware_url": FIRMWARE_URL,
        "asset_version": CURRENT_ASSET_VER,
        "images": image_urls  # ✨ 自動產生的網址清單陣列
    }


# 🎯 API 2: 提供圖片檔案串流下載
@app.get("/api/v1/download/image/{version}/{filename}")
def download_image(version: str, filename: str):
    """
    傳送圖片給 ESP32，自動帶入 Content-Length 供進度條計算
    """
    file_path = os.path.join(BASE_IMAGE_DIR, version, filename)
    
    if not os.path.exists(file_path):
        raise HTTPException(status_code=404, detail=f"Image {filename} not found in {version}")
    
    return FileResponse(
        path=file_path,
        filename=filename,
        media_type="image/bmp"
    )


if __name__ == "__main__":
    import uvicorn
    print(f"🚀 FastAPI 伺服器啟動中...")
    print(f"📡 區域網路 IP: http://{SERVER_IP}:{SERVER_PORT}")
    print(f"📁 當前掃描資料夾: static/images/v{CURRENT_ASSET_VER}/")
    
    # 綁定 0.0.0.0 確保 ESP32 可以連近來
    uvicorn.run(app, host="0.0.0.0", port=SERVER_PORT)