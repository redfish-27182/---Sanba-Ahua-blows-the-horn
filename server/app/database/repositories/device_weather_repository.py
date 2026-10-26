"""設備最新天氣的 SQLAlchemy 資料庫存取。"""

from sqlalchemy import select

from app.database.models.device_weather_model import DeviceWeather
from app.database.session import SessionLocal
from app.schemas.weather_schema import DeviceWeatherInput


def save_latest_device_weather(data: DeviceWeatherInput) -> None:
    """輸入最新天氣，依設備 ID 新增或覆寫資料庫紀錄。"""

    with SessionLocal.begin() as session:
        weather = session.scalar(
            select(DeviceWeather).where(DeviceWeather.device_id == data.device_id)
        )

        if weather is None:
            session.add(DeviceWeather(**data.model_dump()))
            return

        weather.latitude = data.latitude
        weather.longitude = data.longitude
        weather.temperature_c = data.temperature_c
        weather.relative_humidity = data.relative_humidity
        weather.feels_like_c = data.feels_like_c
        weather.weather_code = data.weather_code
        weather.weather_description = data.weather_description
        weather.precipitation_probability = data.precipitation_probability
        weather.observed_at = data.observed_at
        weather.fetched_at = data.fetched_at
        weather.provider = data.provider
