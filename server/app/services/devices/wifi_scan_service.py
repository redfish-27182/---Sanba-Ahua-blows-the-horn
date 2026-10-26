"""將 ESP32 的 Wi-Fi 掃描資料轉成最新設備位置。"""

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
    GoogleGeolocationResponse,
)
from app.schemas.wifi_scan_schema import WifiAccessPointInput, WifiScanInput

logger = logging.getLogger(__name__)

GOOGLE_GEOLOCATION_URL = "https://www.googleapis.com/geolocation/v1/geolocate"
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


async def handle_wifi_scan(device_id: str, payload: bytes) -> None:
    """輸入 MQTT Wi-Fi 掃描 JSON，定位成功後寫入最新設備位置。"""

    try:
        scan = WifiScanInput.model_validate_json(payload)
    except (UnicodeDecodeError, ValidationError) as error:
        logger.warning("設備 %s 的 Wi-Fi 掃描格式無效：%s", device_id, error)
        return

    google_access_points = build_google_access_points(scan.access_points)
    if len(google_access_points) < MINIMUM_ACCESS_POINTS:
        logger.warning(
            "設備 %s 可用的 Wi-Fi AP 少於 %s 個，略過定位。",
            device_id,
            MINIMUM_ACCESS_POINTS,
        )
        return

    api_key_setting = settings.google_geolocation_api_key
    api_key = api_key_setting.get_secret_value() if api_key_setting else ""
    if not api_key:
        logger.error("未設定 GOOGLE_GEOLOCATION_API_KEY，無法定位設備 %s。", device_id)
        return

    # 傳送 Google Geolocation API 規定的 Wi-Fi 欄位；禁止以伺服器 IP 作為替代位置。
    request_body = {
        "considerIp": False,
        "wifiAccessPoints": google_access_points,
    }

    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            response = await client.post(
                GOOGLE_GEOLOCATION_URL,
                params={"key": api_key},
                json=request_body,
            )
            response.raise_for_status()
        result = GoogleGeolocationResponse.model_validate(response.json())
    except httpx.HTTPStatusError as error:
        logger.warning(
            "Google 定位拒絕設備 %s 的請求：HTTP %s。",
            device_id,
            error.response.status_code,
        )
        return
    except (httpx.RequestError, ValidationError, ValueError) as error:
        logger.warning("設備 %s 的 Google 定位失敗：%s", device_id, error)
        return

    location = DeviceLocationInput(
        device_id=device_id,
        latitude=result.location.lat,
        longitude=result.location.lng,
        accuracy_meters=result.accuracy,
        located_at=datetime.now(timezone.utc),
    )
    try:
        save_latest_device_location(location)
    except SQLAlchemyError:
        logger.exception("設備 %s 的定位結果無法寫入資料庫。", device_id)
        return

    logger.info(
        "設備 %s 定位完成：緯度 %.6f，經度 %.6f，誤差約 %.1f 公尺。",
        device_id,
        location.latitude,
        location.longitude,
        location.accuracy_meters,
    )
