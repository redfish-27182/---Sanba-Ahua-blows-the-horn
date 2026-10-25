"""驗證 ESP32 上傳的附近 Wi-Fi 定位資料。"""

from pydantic import BaseModel, Field


class WifiAccessPointInput(BaseModel):
    """一個附近 Wi-Fi 存取點；不接收 SSID。"""

    bssid: str = Field(pattern=r"^[0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5}$")
    rssi: int = Field(ge=-127, le=0)
    channel: int = Field(ge=1, le=14)


class WifiScanInput(BaseModel):
    """單次開機掃描的前幾個訊號最強存取點。"""

    access_points: list[WifiAccessPointInput] = Field(min_length=1, max_length=5)
