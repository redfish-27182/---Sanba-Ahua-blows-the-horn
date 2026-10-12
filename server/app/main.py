"""FastAPI 啟動點：目前只負責啟停 MQTT，不先加入 HTTP 路由。"""

from contextlib import asynccontextmanager

from fastapi import FastAPI

from app.api.mqtt.client import MqttClient


@asynccontextmanager
async def lifespan(app: FastAPI):
    """讓 MQTT 連線跟著 FastAPI 服務一起啟動與關閉。"""
    mqtt_client = MqttClient()
    app.state.mqtt_client = mqtt_client
    await mqtt_client.start()
    try:
        yield
    finally:
        await mqtt_client.stop()


# HTTP API 會在日後網站開始製作時逐步加入；目前保留 FastAPI 作為服務生命週期容器。
app = FastAPI(title="SanBa_Ahou Backend", lifespan=lifespan)
