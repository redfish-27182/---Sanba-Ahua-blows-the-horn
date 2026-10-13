"""目前只處理設備 online / offline 狀態的驗證與資料庫更新。"""

import logging

from pydantic import ValidationError
from sqlalchemy import func
from sqlalchemy.dialects.postgresql import insert

from app.database.models.device_status import DeviceStatus
from app.database.session import SessionLocal
from app.schemas.mqtt import DeviceStatusMessage

logger = logging.getLogger(__name__)


def handle_device_status(device_id: str, payload: bytes) -> None:
    """判斷收到的 payload 是否為 online / offline，並更新該設備最後狀態。"""

    try:
        message = DeviceStatusMessage.from_payload(payload)
    except (UnicodeDecodeError, ValidationError) as error:
        logger.warning("忽略 %s 的無效設備狀態：%s", device_id, error)
        return

    status = message.status
    statement = insert(DeviceStatus).values(device_id=device_id, status=status)
    statement = statement.on_conflict_do_update(
        index_elements=[DeviceStatus.device_id],
        set_={"status": status, "updated_at": func.now()},
    )

    with SessionLocal.begin() as session:
        session.execute(statement)

    logger.info("設備 %s 狀態更新為 %s", device_id, status)
