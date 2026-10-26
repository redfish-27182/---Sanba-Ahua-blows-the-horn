"""Wi-Fi 掃描定位與英文行政區服務的單元測試。"""

import asyncio
import unittest
from unittest.mock import AsyncMock, patch

from pydantic import SecretStr

from app.config.settings import settings
from app.schemas.wifi_scan_schema import WifiAccessPointInput
from app.services.devices import wifi_scan_service


class FakeGeolocationResponse:
    """模擬 Google Wi-Fi 定位成功回應。"""

    def raise_for_status(self) -> None:
        """輸入無，模擬 HTTP 200 不拋出例外。"""

    def json(self) -> dict[str, object]:
        """輸出 Google Geolocation API 的定位 JSON。"""

        return {
            "location": {"lat": 25.033, "lng": 121.5654},
            "accuracy": 42.5,
        }


class FakeGeocodingResponse:
    """模擬 Google 反向地理編碼成功回應。"""

    def raise_for_status(self) -> None:
        """輸入無，模擬 HTTP 200 不拋出例外。"""

    def json(self) -> dict[str, object]:
        """輸出英文國家、行政區與城市的 JSON。"""

        return {
            "results": [
                {
                    "addressComponents": [
                        {
                            "longText": "Taiwan",
                            "shortText": "TW",
                            "types": ["country", "political"],
                        },
                        {
                            "longText": "Taipei City",
                            "shortText": "Taipei City",
                            "types": ["administrative_area_level_1", "political"],
                        },
                        {
                            "longText": "Xinyi District",
                            "shortText": "Xinyi District",
                            "types": ["administrative_area_level_2", "political"],
                        },
                        {
                            "longText": "Taipei",
                            "shortText": "Taipei",
                            "types": ["locality", "political"],
                        },
                    ]
                }
            ]
        }


class FakeGoogleClient:
    """模擬非同步 Google HTTP client，避免測試發出網路請求。"""

    def __init__(self) -> None:
        self.geolocation_params: dict[str, str] | None = None
        self.geolocation_body: dict[str, object] | None = None
        self.geocoding_params: dict[str, float | str] | None = None

    async def __aenter__(self) -> "FakeGoogleClient":
        """輸入無，輸出可供 async with 使用的 client。"""

        return self

    async def __aexit__(self, *_: object) -> None:
        """輸入 context manager 結束參數，測試不需要額外清理。"""

    async def post(
        self,
        _: str,
        *,
        params: dict[str, str],
        json: dict[str, object],
    ) -> FakeGeolocationResponse:
        """輸入 Wi-Fi 定位請求，輸出成功的座標回應。"""

        self.geolocation_params = params
        self.geolocation_body = json
        return FakeGeolocationResponse()

    async def get(
        self,
        _: str,
        *,
        params: dict[str, float | str],
    ) -> FakeGeocodingResponse:
        """輸入反向地理編碼請求，輸出英文行政區回應。"""

        self.geocoding_params = params
        return FakeGeocodingResponse()


class WifiScanLocationTests(unittest.TestCase):
    """確認掃描資料能安全轉為 Google 請求與資料庫資料。"""

    def test_build_google_access_points_deduplicates_and_filters(self) -> None:
        """輸入重複與廣播 BSSID，輸出唯一且可定位的 AP。"""

        access_points = [
            WifiAccessPointInput(bssid="10:22:33:44:55:66", rssi=-50, channel=1),
            WifiAccessPointInput(bssid="10:22:33:44:55:66", rssi=-51, channel=1),
            WifiAccessPointInput(bssid="FF:FF:FF:FF:FF:FF", rssi=-60, channel=6),
        ]

        result = wifi_scan_service.build_google_access_points(access_points)

        self.assertEqual(result, [
            {"macAddress": "10:22:33:44:55:66", "signalStrength": -50, "channel": 1}
        ])

    def test_valid_scan_saves_location_and_english_area_names(self) -> None:
        """輸入兩個可用 AP，輸出包含英文行政區的最新位置。"""

        client = FakeGoogleClient()
        payload = (
            b'{"access_points":[{"bssid":"10:22:33:44:55:66","rssi":-50,"channel":1},'
            b'{"bssid":"10:22:33:44:55:67","rssi":-65,"channel":6}]}'
        )

        with (
            patch.object(settings, "google_geolocation_api_key", SecretStr("test-key")),
            patch.object(wifi_scan_service.httpx, "AsyncClient", return_value=client),
            patch.object(wifi_scan_service, "save_latest_device_location") as save,
            patch.object(
                wifi_scan_service,
                "refresh_weather_for_device",
                new_callable=AsyncMock,
            ) as refresh_weather,
        ):
            asyncio.run(wifi_scan_service.handle_wifi_scan("cyd-01", payload))

        self.assertEqual(client.geolocation_params, {"key": "test-key"})
        self.assertEqual(client.geolocation_body["considerIp"], False)
        self.assertEqual(len(client.geolocation_body["wifiAccessPoints"]), 2)
        self.assertEqual(client.geocoding_params["languageCode"], "en")
        saved = save.call_args.args[0]
        self.assertEqual(saved.device_id, "cyd-01")
        self.assertEqual(saved.latitude, 25.033)
        self.assertEqual(saved.country_code, "TW")
        self.assertEqual(saved.country_name, "Taiwan")
        self.assertEqual(saved.admin_area_level_1, "Taipei City")
        self.assertEqual(saved.locality, "Taipei")
        refresh_weather.assert_awaited_once_with("cyd-01", 25.033, 121.5654)


if __name__ == "__main__":
    unittest.main()
