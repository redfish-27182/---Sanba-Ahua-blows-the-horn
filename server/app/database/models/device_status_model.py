"""儲存每台 ESP32 最後一次回報的上線狀態。"""

from datetime import datetime

from sqlalchemy import DateTime, Integer, String, func
from sqlalchemy.orm import Mapped, mapped_column

from app.database.base import Base


class DeviceStatus(Base):
    """對應 PostgreSQL 的 device_status 資料表。"""

    __tablename__ = "device_status"

    id:         Mapped[int]      = mapped_column(Integer, primary_key=True)
    device_id:  Mapped[str]      = mapped_column(String(100), unique=True, index=True)
    status:     Mapped[str]      = mapped_column(String(16))
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), nullable=False
    )
