"""設備上線狀態 MQTT payload 的驗證規則。"""

from typing import Literal

from pydantic import BaseModel


class DeviceStatusMessage(BaseModel):
    """ESP32 發送到 status topic 的設備上線狀態。"""

    status: Literal["online", "offline"]

    @classmethod
    def from_payload(cls, payload: bytes) -> "DeviceStatusMessage":
        """輸入 MQTT 位元組 payload，輸出已驗證的設備狀態。"""

        return cls(status=payload.decode("utf-8").strip())
