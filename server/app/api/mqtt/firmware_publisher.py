"""發布全域 retained 韌體版本；之後網站 HTTP API 會呼叫這個函式。"""

from app.api.mqtt.client import MqttClient
from app.api.mqtt.topics import FIRMWARE_VERSION_TOPIC


async def publish_firmware_version(mqtt_client: MqttClient, version: str) -> None:
    """將版本號廣播給所有訂閱 ahou/firmware_version 的 ESP32。"""
    clean_version = version.strip()
    if not clean_version:
        raise ValueError("韌體版本不可為空白。")

    await mqtt_client.publish(FIRMWARE_VERSION_TOPIC, clean_version, retained=True)
