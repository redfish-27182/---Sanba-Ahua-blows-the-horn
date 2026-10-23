"""依照 MQTT topic 的樹狀結構，分流 ESP32 傳來的事件。"""

import logging

from app.mqtt.topics                         import DEVICE_EVENTS_TOPIC_ROOT
from app.services.devices.device_status_service import handle_device_status

logger = logging.getLogger(__name__)


async def route_message(topic: str, payload: bytes) -> None:
    """解析 ahou/events/devices/{device_id}/... 並交給對應處理。"""

    device_topic_prefix = f"{DEVICE_EVENTS_TOPIC_ROOT}/"
    if not topic.startswith(device_topic_prefix):
        logger.warning("不支援的 MQTT event topic：%s", topic)
        return

    # 先取得設備 ID，再依其後方的子路徑決定要處理哪一種事件。
    relative_parts = topic.removeprefix(device_topic_prefix).split("/")
    device_id, *event_path = relative_parts

    if not device_id:
        logger.warning("MQTT event topic 缺少設備 ID：%s", topic)
        return

    if event_path == ["status"]:
        handle_device_status(device_id, payload)
        return

    if event_path == ["test", "ping"]:
        # 暫時只記錄訊息，用來確認 ESP32、Broker 與 Router 都正常運作。
        message = payload.decode("utf-8", errors="replace")
        logger.info("收到設備 %s 的測試 ping：%s", device_id, message)
        return

    logger.warning("不支援的設備 MQTT event topic：%s", topic)
