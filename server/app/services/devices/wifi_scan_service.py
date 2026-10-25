"""處理設備上傳的 Wi-Fi 掃描資料；定位功能尚未啟用。"""

import logging

from pydantic import ValidationError

from app.schemas.wifi_scan_schema import WifiScanInput

logger = logging.getLogger(__name__)


def handle_wifi_scan(device_id: str, payload: bytes) -> None:
    """驗證後記錄掃描結果，供下一階段串接定位 API 前核對資料。"""

    try:
        scan = WifiScanInput.model_validate_json(payload)
    except (UnicodeDecodeError, ValidationError) as error:
        logger.warning("設備 %s 的 Wi-Fi 掃描資料無效：%s", device_id, error)
        return

    logger.info(
        "收到設備 %s 的 Wi-Fi 掃描：%s",
        device_id,
        [point.model_dump() for point in scan.access_points],
    )
