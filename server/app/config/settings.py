"""集中從 .env 讀取本機設定；密碼不應直接寫在程式碼中。"""

from pathlib import Path
from urllib.parse import quote_plus

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    """後端使用的資料庫與 MQTT 設定。"""

    model_config = SettingsConfigDict(
        env_file=Path(__file__).resolve().parents[2] / ".env",
        env_file_encoding="utf-8",
        extra="ignore",
    )

    database_host: str = "localhost"
    database_port: int = 5432
    database_name: str = "sanba_ahou"
    database_user: str = "sanba_app"
    database_password: str = ""

    mqtt_host: str = ""
    mqtt_port: int = 8883
    mqtt_username: str = ""
    mqtt_password: str = ""
    mqtt_tls_enabled: bool = True
    mqtt_ca_cert_path: Path | None = None
    mqtt_client_id: str = "sanba-backend"

    @property
    def database_url(self) -> str:
        """組成 SQLAlchemy 使用的 PostgreSQL 連線字串，並安全處理特殊字元密碼。"""
        password = quote_plus(self.database_password)
        return (
            f"postgresql+psycopg://{self.database_user}:{password}"
            f"@{self.database_host}:{self.database_port}/{self.database_name}"
        )


settings = Settings()
