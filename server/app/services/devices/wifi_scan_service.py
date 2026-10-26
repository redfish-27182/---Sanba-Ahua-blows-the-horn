"""將 ESP32 的 Wi-Fi 掃描資料轉成最新設備位置與英文行政區。"""

from datetime import datetime, timezone
import logging

import httpx
from pydantic import ValidationError
from sqlalchemy.exc import SQLAlchemyError

from app.config.settings import settings
from app.database.repositories.device_location_repository import (
    save_latest_device_location,
)
from app.schemas.device_location_schema import (
    DeviceLocationInput,
    DeviceLocationNames,
    GoogleAddressComponent,
    GoogleGeocodingResponse,
    GoogleGeolocationResponse,
)
from app.schemas.wifi_scan_schema import WifiAccessPointInput, WifiScanInput

logger = logging.getLogger(__name__)

GOOGLE_GEOLOCATION_URL = "https://www.googleapis.com/geolocation/v1/geolocate"
GOOGLE_GEOCODING_URL = "https://geocode.googleapis.com/v4/geocode/location"
MINIMUM_ACCESS_POINTS = 2


def build_google_access_points(
    access_points: list[WifiAccessPointInput],
) -> list[dict[str, int | str]]:
    """輸入 ESP32 掃描點，輸出 Google 可接受且不重複的 AP 清單。"""

    google_access_points: list[dict[str, int | str]] = []
    seen_bssids: set[str] = set()

    for point in access_points:
        bssid = point.bssid.upper()
        first_octet = int(bssid[:2], 16)

        # 忽略廣播、多播與本機管理 MAC；Google 不會拿這類位址做 Wi-Fi 定位。
        if bssid == "FF:FF:FF:FF:FF:FF" or first_octet & 0b11:
            continue
        if bssid in seen_bssids:
            continue

        seen_bssids.add(bssid)
        google_access_points.append(
            {
                "macAddress": bssid,
                "signalStrength": point.rssi,
                "channel": point.channel,
            }
        )

    return google_access_points


async def find_coordinates_from_wifi(
    device_id: str,
    access_points: list[dict[str, int | str]],
    api_key: str,
) -> GoogleGeolocationResponse | None:
    """輸入設備 ID、Wi-Fi AP 與金鑰，輸出 Google 定位座標或 None。"""

    request_body = {
        "considerIp": False,
        "wifiAccessPoints": access_points,
    }

    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            response = await client.post(
                GOOGLE_GEOLOCATION_URL,
                params={"key": api_key},
                json=request_body,
            )
            response.raise_for_status()
        return GoogleGeolocationResponse.model_validate(response.json())
    except httpx.HTTPStatusError as error:
        logger.warning(
            "Google 定位拒絕設備 %s 的請求：HTTP %s。",
            device_id,
            error.response.status_code,
        )
    except (httpx.RequestError, ValidationError, ValueError) as error:
        logger.warning("設備 %s 的 Google 定位失敗：%s", device_id, error)

    return None


def find_address_component(
    components: list[GoogleAddressComponent],
    component_type: str,
    *,
    short_name: bool = False,
) -> str | None:
    """輸入地址欄位與類型，輸出第一個對應的英文名稱或代碼。"""

    for component in components:
        if component_type in component.types:
            return component.shortText if short_name else component.longText
    return None


def extract_location_names(response: GoogleGeocodingResponse) -> DeviceLocationNames:
    """輸入 Google 反向地理編碼結果，輸出需保存的英文行政區欄位。"""

    if not response.results:
        return DeviceLocationNames()

    components = response.results[0].addressComponents
    return DeviceLocationNames(
        country_code=find_address_component(components, "country", short_name=True),
        country_name=find_address_component(components, "country"),
        admin_area_level_1=find_address_component(
            components, "administrative_area_level_1"
        ),
        admin_area_level_2=find_address_component(
            components, "administrative_area_level_2"
        ),
        locality=find_address_component(components, "locality"),
    )


async def find_location_names(
    latitude: float,
    longitude: float,
    api_key: str,
) -> DeviceLocationNames:
    """輸入座標與 Google 金鑰，輸出英文國家與行政區；失敗時回傳空欄位。"""

    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            response = await client.get(
                GOOGLE_GEOCODING_URL,
                params={
                    "location.latitude": latitude,
                    "location.longitude": longitude,
                    "languageCode": "en",
                    "key": api_key,
                },
            )
            response.raise_for_status()
        geocoding_result = GoogleGeocodingResponse.model_validate(response.json())
    except httpx.HTTPStatusError as error:
        logger.warning("Google 地名查詢被拒絕：HTTP %s。", error.response.status_code)
        return DeviceLocationNames()
    except (httpx.RequestError, ValidationError, ValueError) as error:
        logger.warning("Google 地名查詢失敗：%s", error)
        return DeviceLocationNames()

    names = extract_location_names(geocoding_result)
    if names.country_name is None:
        logger.warning("Google 沒有回傳座標 %.6f, %.6f 的行政區。", latitude, longitude)
    return names


#--------- 主程式 ---------#
async def handle_wifi_scan(device_id: str, payload: bytes) -> None:
    """輸入 MQTT Wi-Fi 掃描 JSON，保存最新座標與英文行政區。"""

    # 1. 解析 JSON，若格式錯誤則略過。
    try:
        scan = WifiScanInput.model_validate_json(payload)
    except (UnicodeDecodeError, ValidationError) as error:
        logger.warning("設備 %s 的 Wi-Fi 掃描格式無效：%s", device_id, error)
        return

    # 2. 轉換成 Google Geolocation API 所需的格式，若 AP 數量不足則略過。
    google_access_points = build_google_access_points(scan.access_points)
    if len(google_access_points) < MINIMUM_ACCESS_POINTS:
        logger.warning(
            "設備 %s 可用的 Wi-Fi AP 少於 %s 個，略過定位。",
            device_id,
            MINIMUM_ACCESS_POINTS,
        )
        return

    # 3. 檢查 API 金鑰，若未設定則略過。
    api_key_setting = settings.google_geolocation_api_key
    api_key = api_key_setting.get_secret_value() if api_key_setting else ""
    if not api_key:
        logger.error("未設定 GOOGLE_GEOLOCATION_API_KEY，無法定位設備 %s。", device_id)
        return

    # 4. 使用 Google Wi-Fi 定位 API 查詢座標，若失敗則略過。
    geolocation_result = await find_coordinates_from_wifi(
        device_id,
        google_access_points,
        api_key,
    )
    if geolocation_result is None:
        return

    # 5. 使用 Google Geocoding API 查詢英文行政區，若失敗則略過。
    location_names = await find_location_names(
        geolocation_result.location.lat,
        geolocation_result.location.lng,
        api_key,
    )
    location = DeviceLocationInput(
        device_id=device_id,
        latitude=geolocation_result.location.lat,
        longitude=geolocation_result.location.lng,
        accuracy_meters=geolocation_result.accuracy,
        located_at=datetime.now(timezone.utc),
        **location_names.model_dump(),
    )

    # 6. 將定位結果寫入資料庫，若失敗則略過。
    try:
        save_latest_device_location(location)
    except SQLAlchemyError:
        logger.exception("設備 %s 的定位結果無法寫入資料庫。", device_id)
        return

    # 7. 成功寫入資料庫後，紀錄定位結果。
    logger.info(
        "設備 %s 定位完成：緯度 %.6f，經度 %.6f，%s，%s，%s",
        device_id,
        location.latitude,
        location.longitude,
        location.country_name or "未知國家",
        location.admin_area_level_1 or "未知行政區",
        location.admin_area_level_2 or "未知次級行政區",
    )
