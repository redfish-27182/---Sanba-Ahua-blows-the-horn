"""設備狀態資料表的 PostgreSQL 存取操作。"""

from sqlalchemy import func
from sqlalchemy.dialects.postgresql import insert

from app.database.models.device_status_model import DeviceStatus
from app.database.session import SessionLocal


def upsert_device_status(device_id: str, status: str) -> None:
    """新增設備狀態；設備已存在時只更新狀態與最後更新時間。"""

    statement = insert(DeviceStatus).values(device_id=device_id, status=status)
    statement = statement.on_conflict_do_update(
        index_elements=[DeviceStatus.device_id],
        set_={"status": status, "updated_at": func.now()},
    )

    with SessionLocal.begin() as session:
        session.execute(statement)
