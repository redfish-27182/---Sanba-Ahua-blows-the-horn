"""WeatherAPI 回應與設備最新天氣的驗證結構。"""

from datetime import datetime

from pydantic import BaseModel, Field


class WeatherApiCondition(BaseModel):
    """WeatherAPI 回傳的英文天氣狀態。"""

    text: str = Field(max_length=100)
    code: int


class WeatherApiCurrent(BaseModel):
    """WeatherAPI 回傳的即時天氣資料。"""

    last_updated_epoch: int = Field(ge=0)
    temp_c: float
    humidity: int = Field(ge=0, le=100)
    feelslike_c: float
    condition: WeatherApiCondition


class WeatherApiForecastDayValues(BaseModel):
    """WeatherAPI 回傳的當日預報資料。"""

    daily_chance_of_rain: int = Field(ge=0, le=100)


class WeatherApiForecastDay(BaseModel):
    """WeatherAPI 回傳的一日預報。"""

    day: WeatherApiForecastDayValues


class WeatherApiForecast(BaseModel):
    """WeatherAPI 回傳的預報清單。"""

    forecastday: list[WeatherApiForecastDay] = Field(min_length=1)


class WeatherApiResponse(BaseModel):
    """WeatherAPI forecast endpoint 的最小必要回應。"""

    current: WeatherApiCurrent
    forecast: WeatherApiForecast


class DeviceWeatherInput(BaseModel):
    """準備寫入資料庫的單台設備最新天氣。"""

    device_id: str = Field(min_length=1, max_length=100)
    latitude: float = Field(ge=-90, le=90)
    longitude: float = Field(ge=-180, le=180)
    temperature_c: float
    relative_humidity: int = Field(ge=0, le=100)
    feels_like_c: float
    weather_code: int
    weather_description: str = Field(max_length=100)
    precipitation_probability: int = Field(ge=0, le=100)
    observed_at: datetime
    fetched_at: datetime
    provider: str = Field(default="weatherapi", max_length=50)
