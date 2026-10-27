"""以台北時間每小時 :05 更新全部設備天氣的背景 task。"""

import asyncio
import contextlib
from datetime import datetime, timedelta
import logging
from typing import TYPE_CHECKING
from zoneinfo import ZoneInfo

from app.services.weather_service import refresh_all_device_weather

if TYPE_CHECKING:
    from app.mqtt.client import MqttClient

logger = logging.getLogger(__name__)

TAIPEI_TIME_ZONE = ZoneInfo("Asia/Taipei")


def seconds_until_next_refresh(now: datetime | None = None) -> float:
    """輸入目前時間或 None，輸出距離下一個台北時間 :05 的秒數。"""

    local_now = now.astimezone(TAIPEI_TIME_ZONE) if now else datetime.now(TAIPEI_TIME_ZONE)
    next_refresh = local_now.replace(minute=5, second=0, microsecond=0)
    if next_refresh <= local_now:
        next_refresh += timedelta(hours=1)
    return (next_refresh - local_now).total_seconds()


class WeatherRefreshTask:
    """管理開機立即更新與每小時 :05 的天氣背景工作。"""

    def __init__(self, mqtt_client: "MqttClient") -> None:
        """輸入 MQTT client，建立尚未啟動的背景工作狀態。"""

        self._mqtt_client = mqtt_client
        self._task: asyncio.Task[None] | None = None
        self._stop_event = asyncio.Event()

    async def start(self) -> None:
        """輸入無，啟動天氣更新背景工作。"""

        if self._task is None:
            self._task = asyncio.create_task(
                self._run(),
                name="weather-refresh-task",
            )

    async def stop(self) -> None:
        """停止天氣更新背景工作並等待結束。"""

        self._stop_event.set()
        if self._task is not None:
            self._task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await self._task
            self._task = None

    async def _refresh_safely(self, reason: str) -> None:
        """輸入更新原因文字，執行批次更新且保留背景工作。"""

        try:
            logger.info("開始天氣批次更新：%s。", reason)
            await refresh_all_device_weather(self._mqtt_client)
        except asyncio.CancelledError:
            raise
        except Exception:
            logger.exception("天氣批次更新失敗：%s。", reason)

    async def _run(self) -> None:
        """開機更新一次後持續等待每小時台北時間 :05。"""

        await self._mqtt_client.wait_until_connected()
        await self._refresh_safely("後端啟動")

        while not self._stop_event.is_set():
            wait_seconds = seconds_until_next_refresh()
            try:
                await asyncio.wait_for(self._stop_event.wait(), timeout=wait_seconds)
            except TimeoutError:
                await self._refresh_safely("每小時 :05")
