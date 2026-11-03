"""應用程式環境設定。"""

from pathlib import Path
from urllib.parse import quote_plus

from pydantic import SecretStr, field_validator
from pydantic_settings import BaseSettings, SettingsConfigDict


LOCAL_ORIGINS = ("http://localhost:5173", "http://127.0.0.1:5173")


class Settings(BaseSettings):
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

    article_edit_enabled: bool = False
    google_geolocation_api_key: SecretStr | None = None
    weatherapi_api_key: SecretStr | None = None

    mqtt_host: str = ""
    mqtt_port: int = 8883
    mqtt_username: str = ""
    mqtt_password: str = ""
    mqtt_tls_enabled: bool = True
    mqtt_ca_cert_path: Path | None = None
    mqtt_client_id: str = "sanba-backend"

    frontend_url: str = "http://localhost:5173"
    cors_origins: str = ",".join(LOCAL_ORIGINS)
    auth_session_cookie_name: str = "sanba_session"
    auth_cookie_secure: bool = False
    auth_session_ttl_days: int = 30

    @field_validator("mqtt_ca_cert_path", mode="before")
    @classmethod
    def empty_mqtt_ca_cert_path_is_none(cls, value: object) -> object:
        if isinstance(value, str) and not value.strip():
            return None
        return value

    @property
    def database_url(self) -> str:
        password = quote_plus(self.database_password)
        return (
            f"postgresql+psycopg://{self.database_user}:{password}"
            f"@{self.database_host}:{self.database_port}/{self.database_name}"
        )

    @property
    def allowed_origins(self) -> tuple[str, ...]:
        configured_origins = tuple(
            origin.strip().rstrip("/")
            for origin in self.cors_origins.split(",")
            if origin.strip()
        )
        return tuple(dict.fromkeys((*LOCAL_ORIGINS, self.frontend_url.rstrip("/"), *configured_origins)))

    @property
    def auth_session_ttl_seconds(self) -> int:
        return self.auth_session_ttl_days * 24 * 60 * 60


settings = Settings()
