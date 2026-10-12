"""Alembic migration 設定：讀取 .env 的資料庫連線，並載入所有 model。"""

from logging.config import fileConfig

from alembic import context
from sqlalchemy import engine_from_config, pool

from app.config.settings import settings
from app.database.base import Base
from app.database.models import DeviceStatus  # noqa: F401 - 讓 Alembic 載入資料表 model

config = context.config
if config.config_file_name is not None:
    fileConfig(config.config_file_name)

# 密碼可能含 %，需轉義才能安全寫進 ConfigParser。
config.set_main_option("sqlalchemy.url", settings.database_url.replace("%", "%%"))
target_metadata = Base.metadata


def run_migrations_offline() -> None:
    """產生 SQL 文字時使用，不連線到實際資料庫。"""
    context.configure(
        url=config.get_main_option("sqlalchemy.url"),
        target_metadata=target_metadata,
        literal_binds=True,
        dialect_opts={"paramstyle": "named"},
    )

    with context.begin_transaction():
        context.run_migrations()


def run_migrations_online() -> None:
    """一般 upgrade 使用：連線後依序套用 migration。"""
    connectable = engine_from_config(
        config.get_section(config.config_ini_section, {}),
        prefix="sqlalchemy.",
        poolclass=pool.NullPool,
    )

    with connectable.connect() as connection:
        context.configure(connection=connection, target_metadata=target_metadata)
        with context.begin_transaction():
            context.run_migrations()


if context.is_offline_mode():
    run_migrations_offline()
else:
    run_migrations_online()
