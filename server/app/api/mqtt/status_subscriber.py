"""處理 ESP32 發出的 online / offline retained 狀態訊息。"""

import logging

from pydantic import ValidationError

from app.api.mqtt.topics import device_id_from_status_topic
from app.schemas.mqtt import DeviceStatusMessage
from app.services.mqtt_service import update_device_status

logger = logging.getLogger(__name__)


async def handle_status_message(topic: str, payload: bytes) -> None:
    """驗證 MQTT topic 與 payload 後，將設備目前狀態寫入資料庫。"""
    device_id = device_id_from_status_topic(topic)
    if device_id is None:
        logger.warning("忽略不符合設備狀態格式的 topic：%s", topic)
        return

    try:
        message = DeviceStatusMessage.from_payload(payload)
    except (UnicodeDecodeError, ValidationError) as error:
        logger.warning("忽略 %s 的無效設備狀態：%s", device_id, error)
        return

    update_device_status(device_id, message.status)
    logger.info("設備 %s 狀態更新為 %s", device_id, message.status)
