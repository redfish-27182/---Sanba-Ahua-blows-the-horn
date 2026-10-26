"""儲存每台設備最後一次成功定位的 ORM model。"""

from datetime import datetime

from sqlalchemy import DateTime, Float, Integer, String, func
from sqlalchemy.orm import Mapped, mapped_column

from app.database.base import Base


class DeviceLocation(Base):
    """對應 device_locations 資料表的最新設備位置。"""

    __tablename__ = "device_locations"

    id: Mapped[int]                = mapped_column(Integer, primary_key=True)
    device_id: Mapped[str]         = mapped_column(String(100), unique=True, index=True)
    latitude: Mapped[float]        = mapped_column(Float)
    longitude: Mapped[float]       = mapped_column(Float)
    accuracy_meters: Mapped[float] = mapped_column(Float)
    provider: Mapped[str]          = mapped_column(String(50))
    country_code: Mapped[str | None] = mapped_column(String(10))
    country_name: Mapped[str | None] = mapped_column(String(100))
    admin_area_level_1: Mapped[str | None] = mapped_column(String(150))
    admin_area_level_2: Mapped[str | None] = mapped_column(String(150))
    locality: Mapped[str | None] = mapped_column(String(150))
    located_at: Mapped[datetime]   = mapped_column(DateTime(timezone=True), nullable=False)
    updated_at: Mapped[datetime]   = mapped_column(
        DateTime(timezone=True),
        server_default=func.now(),
        onupdate=func.now(),
        nullable=False,
    )
