"""儲存每台設備最後一次成功取得的天氣。"""

from datetime import datetime

from sqlalchemy import DateTime, Float, Integer, String, func
from sqlalchemy.orm import Mapped, mapped_column

from app.database.base import Base


class DeviceWeather(Base):
    """對應 device_weather 資料表的最新設備天氣。"""

    __tablename__ = "device_weather"

    id: Mapped[int] = mapped_column(Integer, primary_key=True)
    device_id: Mapped[str] = mapped_column(String(100), unique=True, index=True)
    latitude: Mapped[float] = mapped_column(Float)
    longitude: Mapped[float] = mapped_column(Float)
    temperature_c: Mapped[float] = mapped_column(Float)
    relative_humidity: Mapped[int] = mapped_column(Integer)
    feels_like_c: Mapped[float] = mapped_column(Float)
    weather_code: Mapped[int] = mapped_column(Integer)
    weather_description: Mapped[str] = mapped_column(String(100))
    precipitation_probability: Mapped[int] = mapped_column(Integer)
    observed_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), nullable=False)
    fetched_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), nullable=False)
    provider: Mapped[str] = mapped_column(String(50))
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True),
        server_default=func.now(),
        onupdate=func.now(),
        nullable=False,
    )
