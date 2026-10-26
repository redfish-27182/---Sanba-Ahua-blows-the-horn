"""處理設備上線與離線狀態的業務規則。"""

import logging

from pydantic import ValidationError

from app.database.repositories.device_status_repository import upsert_device_status
from app.schemas.device_status_schema import DeviceStatusMessage

logger = logging.getLogger(__name__)


def handle_device_status(device_id: str, payload: bytes) -> None:
    """驗證設備狀態訊息後，更新該設備的最新狀態。"""

    try:
        message = DeviceStatusMessage.from_payload(payload)
    except (UnicodeDecodeError, ValidationError) as error:
        logger.warning("設備 %s 的狀態訊息無效：%s", device_id, error)
        return

    upsert_device_status(device_id, message.status)
    logger.info("設備 %s 狀態已更新為 %s", device_id, message.status)
