"""Wi-Fi 掃描定位服務的單元測試。"""

import asyncio
import unittest
from unittest.mock import patch

from pydantic import SecretStr

from app.config.settings import settings
from app.schemas.wifi_scan_schema import WifiAccessPointInput
from app.services.devices import wifi_scan_service


class FakeGoogleResponse:
    """模擬 Google 成功回傳的 HTTP 回應。"""

    def raise_for_status(self) -> None:
        """輸入無，模擬 HTTP 200 不拋出例外。"""

    def json(self) -> dict[str, object]:
        """輸出 Google Geolocation API 的定位 JSON。"""

        return {
            "location": {"lat": 25.033, "lng": 121.5654},
            "accuracy": 42.5,
        }


class FakeGoogleClient:
    """模擬非同步 Google HTTP client，避免測試發出網路請求。"""

    def __init__(self) -> None:
        self.params: dict[str, str] | None = None
        self.body: dict[str, object] | None = None

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
    ) -> FakeGoogleResponse:
        """輸入 Google 請求參數，輸出預設成功的定位回應。"""

        self.params = params
        self.body = json
        return FakeGoogleResponse()


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

    def test_valid_scan_saves_latest_google_location(self) -> None:
        """輸入兩個可用 AP，輸出寫入最新位置的 repository 呼叫。"""

        client = FakeGoogleClient()
        payload = (
            b'{"access_points":[{"bssid":"10:22:33:44:55:66","rssi":-50,"channel":1},'
            b'{"bssid":"10:22:33:44:55:67","rssi":-65,"channel":6}]}'
        )

        with (
            patch.object(settings, "google_geolocation_api_key", SecretStr("test-key")),
            patch.object(wifi_scan_service.httpx, "AsyncClient", return_value=client),
            patch.object(wifi_scan_service, "save_latest_device_location") as save,
        ):
            asyncio.run(wifi_scan_service.handle_wifi_scan("cyd-01", payload))

        self.assertEqual(client.params, {"key": "test-key"})
        self.assertEqual(client.body["considerIp"], False)
        self.assertEqual(len(client.body["wifiAccessPoints"]), 2)
        saved = save.call_args.args[0]
        self.assertEqual(saved.device_id, "cyd-01")
        self.assertEqual(saved.latitude, 25.033)
        self.assertEqual(saved.accuracy_meters, 42.5)


if __name__ == "__main__":
    unittest.main()
