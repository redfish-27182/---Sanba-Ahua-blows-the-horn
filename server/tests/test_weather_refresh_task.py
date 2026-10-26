"""每小時天氣更新排程的單元測試。"""

import unittest
from datetime import datetime
from zoneinfo import ZoneInfo

from app.tasks.weather_refresh_task import (
    TAIPEI_TIME_ZONE,
    seconds_until_next_refresh,
)


class WeatherRefreshTaskTests(unittest.TestCase):
    """確認更新時間固定在台北時間每小時 :05。"""

    def test_waits_until_current_hour_five_minutes(self) -> None:
        """輸入台北時間 10:03，輸出距離 10:05 的 120 秒。"""

        now = datetime(2026, 10, 26, 10, 3, tzinfo=TAIPEI_TIME_ZONE)

        self.assertEqual(seconds_until_next_refresh(now), 120)

    def test_waits_until_next_hour_after_five_minutes(self) -> None:
        """輸入台北時間 10:06，輸出距離 11:05 的秒數。"""

        now = datetime(2026, 10, 26, 10, 6, tzinfo=ZoneInfo("Asia/Taipei"))

        self.assertEqual(seconds_until_next_refresh(now), 3540)


if __name__ == "__main__":
    unittest.main()
