"""韌體版本發布的應用服務。"""

from app.api.mqtt.client import MqttClient


FIRMWARE_VERSION_TOPIC = "ahou/firmware_version"


async def publish_firmware_version(mqtt_client: MqttClient, version: str) -> None:
    """將韌體版本以 retained MQTT 訊息發布給裝置。"""

    clean_version = version.strip()
    if not clean_version:
        raise ValueError("韌體版本不可為空白。")

    await mqtt_client.publish(
        FIRMWARE_VERSION_TOPIC,
        clean_version,
        retained=True,
    )
