"""目前只處理設備 online / offline 狀態的資料庫更新。"""

from sqlalchemy import func
from sqlalchemy.dialects.postgresql import insert

from app.database.models.device_status import DeviceStatus
from app.database.session import SessionLocal


def update_device_status(device_id: str, status: str) -> None:
    """新增或更新單一設備最後狀態，避免同一設備產生重複資料列。"""
    statement = insert(DeviceStatus).values(device_id=device_id, status=status)
    statement = statement.on_conflict_do_update(
        index_elements=[DeviceStatus.device_id],
        set_={"status": status, "updated_at": func.now()},
    )

    with SessionLocal.begin() as session:
        session.execute(statement)
