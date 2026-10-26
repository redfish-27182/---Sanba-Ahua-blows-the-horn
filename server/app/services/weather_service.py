"""向 WeatherAPI 取得並保存設備最新天氣。"""

from datetime import datetime, timezone
import logging

import httpx
from pydantic import ValidationError
from sqlalchemy.exc import SQLAlchemyError

from app.config.settings import settings
from app.database.repositories.device_location_repository import (
    get_all_device_locations,
)
from app.database.repositories.device_weather_repository import (
    save_latest_device_weather,
)
from app.schemas.weather_schema import DeviceWeatherInput, WeatherApiResponse

logger = logging.getLogger(__name__)

WEATHERAPI_FORECAST_URL = "https://api.weatherapi.com/v1/forecast.json"

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


async def refresh_weather_for_device(
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
    try:
        save_latest_device_weather(data)
    except SQLAlchemyError:
        logger.exception("設備 %s 的天氣結果無法寫入資料庫。", device_id)
        return False

    # 4. 成功更新後顯示訊息。
    logger.info(
        "設備 %s 天氣更新完成：%.1f°C，濕度 %s%%，%s。",
        device_id,
        data.temperature_c,
        data.relative_humidity,
        data.weather_description,
    )
    return True


async def refresh_all_device_weather() -> None:
    """依所有已知設備座標逐台更新最新天氣。"""

    locations = get_all_device_locations()
    if not locations:
        logger.info("沒有已定位設備，略過天氣批次更新。")
        return

    logger.info("開始更新 %s 台已定位設備的天氣。", len(locations))
    for location in locations:
        await refresh_weather_for_device(
            location.device_id,
            location.latitude,
            location.longitude,
        )
