"""WeatherAPI 天氣服務的單元測試。"""

import asyncio
import unittest
from unittest.mock import patch

from pydantic import SecretStr

from app.config.settings import settings
from app.services import weather_service


class FakeWeatherApiResponse:
    """模擬 WeatherAPI 成功回傳的 HTTP 回應。"""

    def raise_for_status(self) -> None:
        """輸入無，模擬 HTTP 200 不拋出例外。"""

    def json(self) -> dict[str, object]:
        """輸出即時天氣與當日降雨機率 JSON。"""

        return {
            "current": {
                "last_updated_epoch": 1761473100,
                "temp_c": 26.5,
                "humidity": 74,
                "feelslike_c": 29.1,
                "condition": {"text": "Partly cloudy", "code": 1003},
            },
            "forecast": {
                "forecastday": [
                    {"day": {"daily_chance_of_rain": 35}}
                ]
            },
        }


class FakeWeatherApiClient:
    """模擬非同步 WeatherAPI client，避免測試發出網路請求。"""

    def __init__(self) -> None:
        self.params: dict[str, int | str] | None = None

    async def __aenter__(self) -> "FakeWeatherApiClient":
        """輸入無，輸出可供 async with 使用的 client。"""

        return self

    async def __aexit__(self, *_: object) -> None:
        """輸入 context manager 結束參數，測試不需要額外清理。"""

    async def get(
        self,
        _: str,
        *,
        params: dict[str, int | str],
    ) -> FakeWeatherApiResponse:
        """輸入 WeatherAPI 請求參數，輸出成功天氣回應。"""

        self.params = params
        return FakeWeatherApiResponse()


class WeatherServiceTests(unittest.TestCase):
    """確認 WeatherAPI 資料會轉成設備最新天氣。"""

    def test_refresh_weather_saves_latest_data(self) -> None:
        """輸入設備與座標，輸出保存轉換後天氣資料的呼叫。"""

        client = FakeWeatherApiClient()
        with (
            patch.object(settings, "weatherapi_api_key", SecretStr("test-key")),
            patch.object(weather_service.httpx, "AsyncClient", return_value=client),
            patch.object(weather_service, "save_latest_device_weather") as save,
        ):
            result = asyncio.run(
                weather_service.refresh_weather_for_device(
                    "cyd-01",
                    25.033,
                    121.5654,
                )
            )

        self.assertTrue(result)
        self.assertEqual(client.params["key"], "test-key")
        self.assertEqual(client.params["q"], "25.033,121.5654")
        self.assertEqual(client.params["days"], 1)
        saved = save.call_args.args[0]
        self.assertEqual(saved.device_id, "cyd-01")
        self.assertEqual(saved.temperature_c, 26.5)
        self.assertEqual(saved.relative_humidity, 74)
        self.assertEqual(saved.feels_like_c, 29.1)
        self.assertEqual(saved.weather_description, "Partly cloudy")
        self.assertEqual(saved.precipitation_probability, 35)


if __name__ == "__main__":
    unittest.main()
