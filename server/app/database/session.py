"""建立 PostgreSQL 連線與資料庫 session。"""

from sqlalchemy import create_engine
from sqlalchemy.orm import Session, sessionmaker

from app.config.settings import settings

# pool_pre_ping 可在長時間運行後先檢查連線，降低斷線後直接查詢失敗的機率。
engine = create_engine(settings.database_url, pool_pre_ping=True)
SessionLocal = sessionmaker(bind = engine, class_ = Session, expire_on_commit = False)
