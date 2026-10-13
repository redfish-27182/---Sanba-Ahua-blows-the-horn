"""收到 ahou/# 訊息後，依 topic 導向對應的處理程式。"""

import logging

from app.mqtt.topics            import FIRMWARE_VERSION_TOPIC
from app.mqtt.topics            import device_id_from_status_topic
from app.services.mqtt_service  import handle_device_status

logger = logging.getLogger(__name__)


async def route_message(topic: str, payload: bytes) -> None:
    """集中處理 topic 分派；新增訊息種類時在這裡增加對應分支。"""

    # Router 只負責辨認路徑；online / offline 的驗證與資料庫處理由 service 處理。
    device_id = device_id_from_status_topic(topic)
    if device_id is not None:
        handle_device_status(device_id, payload)
        return

    # 後端會發布全域版本；訂閱 ahou/# 後也可能收到這則 retained 訊息。
    # 目前後端不需要消費它，因此明確略過，避免被當成未知訊息。
    if topic == FIRMWARE_VERSION_TOPIC:
        logger.debug("略過韌體版本 topic：%s", topic)
        return

    logger.warning("尚未對應的 MQTT topic：%s", topic)
