"""MQTT 用戶端：負責長時間訂閱、斷線後重連，以及提供發布方法。"""

import asyncio 
import contextlib 
import logging 
import ssl 

import aiomqtt

from app.mqtt.router                import route_message
from app.mqtt.topics                import EVENTS_TOPIC_FILTER
from app.config.settings            import settings

logger = logging.getLogger(__name__)


class MqttClient:
    """MQTT 用戶端：負責長時間訂閱、斷線後重連，以及提供發布方法。"""

    def __init__(self) -> None:
        self._client: aiomqtt.Client | None = None
        self._task: asyncio.Task[None] | None = None
        self._stop_event = asyncio.Event()
        self._connected_event = asyncio.Event()

    async def start(self) -> None:
        """MQTT，啟動!!"""

        if self._task is None:
            self._task = asyncio.create_task(
                self._listen_forever(), 
                name="mqtt-listener"
            )

    async def stop(self) -> None:
        """取消背景工作"""
        self._stop_event.set()
        if self._task is not None:
            self._task.cancel()
            # contextlib.suppress() 會忽略 asyncio.CancelledError
            with contextlib.suppress(asyncio.CancelledError):
                await self._task
            self._task = None

    async def wait_until_connected(self) -> None:
        """等待 MQTT 連線與 events 訂閱完成。"""

        await self._connected_event.wait()

    async def publish(
        self,
        topic: str,
        payload: str,
        *,
        retained: bool = False,
        qos: int = 1,
    ) -> None:
        """發布 MQTT 訊息"""

        if self._client is None:
            raise RuntimeError("MQTT 尚未連線，無法發布訊息。")

        await self._client.publish(
            topic, 
            payload = payload.encode("utf-8"), 
            qos = qos,
            retain = retained
        )

    def _ssl_context(self) -> ssl.SSLContext | None:
        """建立 TLS 驗證環境"""

        if not settings.mqtt_tls_enabled:
            return None

        context = ssl.create_default_context()
        if settings.mqtt_ca_cert_path:
            context.load_verify_locations(
                cafile=str(settings.mqtt_ca_cert_path)
            )
        return context

    async def _listen_forever(self) -> None:
        """訂閱設備狀態，斷線時每五秒重試，直到服務關閉。"""
        while not self._stop_event.is_set():
            try:
                async with aiomqtt.Client(
                    hostname    = settings.mqtt_host,
                    port        = settings.mqtt_port,
                    username    = settings.mqtt_username,
                    password    = settings.mqtt_password,
                    identifier  = settings.mqtt_client_id,
                    tls_context = self._ssl_context(),
                ) as client:
                    self._client = client
                    await client.subscribe( EVENTS_TOPIC_FILTER, qos=1 )

                    logger.info("已訂閱 MQTT topic：%s", EVENTS_TOPIC_FILTER)
                    self._connected_event.set()

                    async for message in client.messages:
                        await route_message(
                            str(message.topic),
                            bytes(message.payload),
                            self,
                        )
            
            except asyncio.CancelledError:
                raise
            except aiomqtt.MqttError as error:
                logger.warning("MQTT 連線中斷：%s；五秒後重試。", error)
            except Exception:
                logger.exception("MQTT 背景工作發生未預期錯誤；五秒後重試。")
            finally:
                self._client = None
                self._connected_event.clear()

            try:
                await asyncio.wait_for(
                    self._stop_event.wait(), 
                    timeout = 5
                )
            except TimeoutError:
                pass
