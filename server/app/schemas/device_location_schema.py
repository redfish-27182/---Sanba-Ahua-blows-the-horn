"""Google 定位回應與設備定位資料的驗證結構。"""

from datetime import datetime

from pydantic import BaseModel, Field


class GoogleLocation(BaseModel):
    """Google 回傳的地理座標。"""

    lat: float = Field(ge=-90, le=90)
    lng: float = Field(ge=-180, le=180)


class GoogleGeolocationResponse(BaseModel):
    """Google Geolocation API 成功回應。"""

    location: GoogleLocation
    accuracy: float = Field(ge=0)


class DeviceLocationInput(BaseModel):
    """準備寫入資料庫的最新設備定位結果。"""

    device_id: str = Field(min_length=1, max_length=100)
    latitude: float = Field(ge=-90, le=90)
    longitude: float = Field(ge=-180, le=180)
    accuracy_meters: float = Field(ge=0)
    provider: str = Field(default="google_geolocation", max_length=50)
    located_at: datetime
