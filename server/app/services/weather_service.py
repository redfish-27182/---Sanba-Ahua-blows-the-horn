"""向 WeatherAPI 取得並保存設備最新天氣。"""

# 標準庫
from datetime import datetime, timezone
import logging
from typing import TYPE_CHECKING
from zoneinfo import ZoneInfo, ZoneInfoNotFoundError

# 第三方庫
import httpx
from pydantic import ValidationError
from sqlalchemy.exc import SQLAlchemyError

from app.config.settings import settings
from app.database.repositories.device_location_repository import get_all_device_locations
from app.database.repositories.device_weather_repository import save_latest_device_weather
from app.mqtt.topics import device_weather_topic
from app.schemas.weather_schema import (
    DeviceWeatherInput,
    DeviceWeatherMqttPayload,
    WeatherApiResponse,
)

if TYPE_CHECKING:
    from app.mqtt.client import MqttClient

logger = logging.getLogger(__name__)

WEATHERAPI_FORECAST_URL = "https://api.weatherapi.com/v1/forecast.json"


def timezone_offset_minutes(timezone_id: str, observed_at: datetime) -> int | None:
    """輸入 IANA 時區與 UTC 觀測時間，輸出當下 UTC 偏移分鐘數或 None。"""
    try:
        offset = observed_at.astimezone(ZoneInfo(timezone_id)).utcoffset()
    except ZoneInfoNotFoundError:
        logger.warning("WeatherAPI 回傳未知時區：%s", timezone_id)
        return None

    if offset is None:
        logger.warning("WeatherAPI 時區 %s 沒有可用 UTC 偏移。", timezone_id)
        return None

    return int(offset.total_seconds() // 60)

async def fetch_weather_from_weatherapi(
    latitude: float,
    longitude: float,
    api_key: str,
) -> WeatherApiResponse | None:
    """輸入座標與 WeatherAPI 金鑰，輸出即時天氣與當日預報或 None。"""
    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            response = await client.get(
                WEATHERAPI_FORECAST_URL,
                params={
                    "key": api_key,
                    "q": f"{latitude},{longitude}",
                    "days": 1,
                    "aqi": "no",
                    "alerts": "no",
                    "lang": "en",
                },
            )
            response.raise_for_status()
        return WeatherApiResponse.model_validate(response.json())
    
    except httpx.HTTPStatusError as error:
        logger.warning("WeatherAPI 查詢被拒絕：HTTP %s。", error.response.status_code)
    except (httpx.RequestError, ValidationError, ValueError) as error:
        logger.warning("WeatherAPI 查詢失敗：%s", error)

    return None


async def publish_weather_to_device(
    mqtt_client: "MqttClient",
    data: DeviceWeatherInput,
    utc_offset_minutes: int,
) -> None:
    """輸入 MQTT client、已保存天氣及 UTC 偏移，發布設備專屬 retained 訊息。"""

    payload = DeviceWeatherMqttPayload(
        at=int(data.observed_at.timestamp()),
        t=data.temperature_c,
        h=data.relative_humidity,
        f=data.feels_like_c,
        c=data.weather_code,
        p=data.precipitation_probability,
        z=utc_offset_minutes,
    )

    try:
        await mqtt_client.publish(
            device_weather_topic(data.device_id),
            payload.model_dump_json(),
            retained=True,
            qos=1,
        )
    except Exception as error:
        logger.warning("設備 %s 的天氣 MQTT 發布失敗：%s", data.device_id, error)


async def refresh_weather_for_device(
    mqtt_client: "MqttClient",
    device_id: str,
    latitude: float,
    longitude: float,
) -> bool:
    """輸入設備 ID 與座標，查詢並保存該設備的最新天氣。"""

    # 1. 取得 WeatherAPI 金鑰，若未設定則無法更新。
    api_key_setting = settings.weatherapi_api_key
    api_key = api_key_setting.get_secret_value() if api_key_setting else ""
    if not api_key:
        logger.error("未設定 WEATHERAPI_API_KEY，無法更新設備 %s 的天氣。", device_id)
        return False

    # 2. 查詢 WeatherAPI，若失敗則不更新。
    weather = await fetch_weather_from_weatherapi(latitude, longitude, api_key)
    if weather is None:
        return False

    # 3. 將查詢結果轉換為資料庫模型並保存，若失敗則不更新。
    current = weather.current
    data = DeviceWeatherInput(
        device_id=device_id,
        latitude=latitude,
        longitude=longitude,
        temperature_c=current.temp_c,
        relative_humidity=current.humidity,
        feels_like_c=current.feelslike_c,
        weather_code=current.condition.code,
        weather_description=current.condition.text,
        precipitation_probability=(
            weather.forecast.forecastday[0].day.daily_chance_of_rain
        ),
        observed_at=datetime.fromtimestamp(current.last_updated_epoch, timezone.utc),
        fetched_at=datetime.now(timezone.utc),
    )
    utc_offset_minutes = timezone_offset_minutes(weather.location.tz_id, data.observed_at)
    if utc_offset_minutes is None:
        logger.error("設備 %s 的時區無法轉換，略過本次天氣更新。", device_id)
        return False
    try:
        save_latest_device_weather(data)
    except SQLAlchemyError:
        logger.exception("設備 %s 的天氣結果無法寫入資料庫。", device_id)
        return False

    # 4. 發布 retained MQTT 訊息給設備。
    await publish_weather_to_device(mqtt_client, data, utc_offset_minutes)

    # 5. 成功更新後顯示訊息。
    logger.info(
        "設備 %s 天氣更新完成：%.1f°C，濕度 %s%%，%s，UTC%+d 分鐘。",
        device_id,
        data.temperature_c,
        data.relative_humidity,
        data.weather_description,
        utc_offset_minutes,
    )
    return True


async def refresh_all_device_weather(mqtt_client: "MqttClient") -> None:
    """依所有已知設備座標逐台更新最新天氣。"""

    locations = get_all_device_locations()
    if not locations:
        logger.info("沒有已定位設備，略過天氣批次更新。")
        return

    logger.info("開始更新 %s 台已定位設備的天氣。", len(locations))
    for location in locations:
        await refresh_weather_for_device(
            mqtt_client,
            location.device_id,
            location.latitude,
            location.longitude,
        )
