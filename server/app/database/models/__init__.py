"""資料表模型。"""

from app.database.models.device_status_model import DeviceStatus
from app.database.models.article_model import Article
from app.database.models.device_location_model import DeviceLocation

__all__ = ["Article", "DeviceLocation", "DeviceStatus"]
