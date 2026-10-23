"""MQTT payload 的最小驗證規則。"""

from typing import Literal

from pydantic import BaseModel


class DeviceStatusMessage(BaseModel):
    """ESP32 在設備狀態 topic 發送的純文字內容。"""

    status: Literal["online", "offline"]

    @classmethod
    def from_payload(cls, payload: bytes) -> "DeviceStatusMessage":
        """將 MQTT 位元組內容轉成已驗證的狀態資料。"""
        return cls(status=payload.decode("utf-8").strip())
