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


class GoogleAddressComponent(BaseModel):
    """Google 回傳的一個地址組成欄位。"""

    longText: str
    shortText: str
    types: list[str] = Field(default_factory=list)


class GoogleGeocodingResult(BaseModel):
    """Google 反向地理編碼的一筆結果。"""

    addressComponents: list[GoogleAddressComponent] = Field(default_factory=list)


class GoogleGeocodingResponse(BaseModel):
    """Google Geocoding API 成功回應。"""

    results: list[GoogleGeocodingResult] = Field(default_factory=list)


class DeviceLocationNames(BaseModel):
    """從反向地理編碼取得的英文區域名稱。"""

    country_code: str | None = Field(default=None, max_length=10)
    country_name: str | None = Field(default=None, max_length=100)
    admin_area_level_1: str | None = Field(default=None, max_length=150)
    admin_area_level_2: str | None = Field(default=None, max_length=150)
    locality: str | None = Field(default=None, max_length=150)


class DeviceLocationInput(DeviceLocationNames):
    """準備寫入資料庫的最新設備定位結果。"""

    device_id: str = Field(min_length=1, max_length=100)
    latitude: float = Field(ge=-90, le=90)
    longitude: float = Field(ge=-180, le=180)
    accuracy_meters: float = Field(ge=0)
    provider: str = Field(default="google_geolocation", max_length=50)
    located_at: datetime
