"""設備最新位置的 SQLAlchemy 資料庫存取。"""

from sqlalchemy import select

from app.database.models.device_location_model import DeviceLocation
from app.database.session import SessionLocal
from app.schemas.device_location_schema import DeviceLocationInput


def save_latest_device_location(data: DeviceLocationInput) -> None:
    """依設備 ID 新增或覆寫最後一次成功定位。"""

    with SessionLocal.begin() as session:
        location = session.scalar(
            select(DeviceLocation).where(DeviceLocation.device_id == data.device_id)
        )

        if location is None:
            session.add(DeviceLocation(**data.model_dump()))
            return

        location.latitude = data.latitude
        location.longitude = data.longitude
        location.accuracy_meters = data.accuracy_meters
        location.provider = data.provider
        location.country_code = data.country_code
        location.country_name = data.country_name
        location.admin_area_level_1 = data.admin_area_level_1
        location.admin_area_level_2 = data.admin_area_level_2
        location.locality = data.locality
        location.located_at = data.located_at
