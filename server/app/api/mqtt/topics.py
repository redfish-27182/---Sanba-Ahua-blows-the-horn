"""集中管理目前使用的 MQTT topic，避免字串散落在程式各處。"""

DEVICE_STATUS_TOPIC_FILTER = "ahou/device_status/+/status"
DEVICE_STATUS_TOPIC_PREFIX = "ahou/device_status/"
DEVICE_STATUS_TOPIC_SUFFIX = "/status"


def device_id_from_status_topic(topic: str) -> str | None:
    """從 ahou/device_status/{device_id}/status 取出 device_id。"""
    if not topic.startswith(DEVICE_STATUS_TOPIC_PREFIX) or not topic.endswith(
        DEVICE_STATUS_TOPIC_SUFFIX
    ):
        return None

    device_id = topic[len(DEVICE_STATUS_TOPIC_PREFIX) : -len(DEVICE_STATUS_TOPIC_SUFFIX)]
    return device_id if device_id and "/" not in device_id else None
