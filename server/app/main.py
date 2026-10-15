"""FastAPI 應用程式進入點，負責管理 MQTT 的生命週期。"""

import asyncio
import contextlib
import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI

from app.mqtt.client import MqttClient
from app.services.firmware_service import publish_firmware_version

logger = logging.getLogger(__name__)


def configure_application_logging() -> None:
    """讓 app 底下的 INFO log 顯示在啟動 Uvicorn 的終端機。"""

    app_logger = logging.getLogger("app")
    app_logger.setLevel(logging.INFO)
    app_logger.propagate = False

    # Uvicorn 的 reload 模式可能重複載入此模組，避免重複加入 handler。
    if app_logger.handlers:
        return

    handler = logging.StreamHandler()
    handler.setFormatter(
        logging.Formatter("%(asctime)s | %(levelname)s | %(name)s | %(message)s")
    )
    app_logger.addHandler(handler)


configure_application_logging()

# 資料庫尚未提供版本資料前，先用固定版本驗證 MQTT 廣播流程。
TEST_FIRMWARE_VERSION = "0.1.2"


async def publish_firmware_version_when_connected(
    mqtt_client: MqttClient,
) -> None:
    """等待 MQTT 連線完成後，發布 retained 測試版本。"""

    await mqtt_client.wait_until_connected()
    await publish_firmware_version(mqtt_client, TEST_FIRMWARE_VERSION)
    logger.info("已發布測試韌體版本：%s", TEST_FIRMWARE_VERSION)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """於 FastAPI 啟動與關閉時管理 MQTT 背景連線。"""

    mqtt_client = MqttClient()
    app.state.mqtt_client = mqtt_client
    await mqtt_client.start()

    # 背景等待可避免 Broker 暫時離線時，整個 FastAPI 無法啟動。
    firmware_version_task = asyncio.create_task(
        publish_firmware_version_when_connected(mqtt_client),
        name="firmware-version-publisher",
    )

    try:
        yield
    finally:
        firmware_version_task.cancel()
        with contextlib.suppress(asyncio.CancelledError):
            await firmware_version_task
        await mqtt_client.stop()


# HTTP API 日後會在這個 FastAPI 應用程式上新增。
app = FastAPI(title="SanBa_Ahou Backend", lifespan=lifespan)
