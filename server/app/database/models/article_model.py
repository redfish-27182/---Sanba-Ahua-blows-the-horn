"""文章 model：文字格式使用 Tiptap JSON 保存。"""
from datetime import datetime
from typing import Any

from sqlalchemy import DateTime, Integer, String, func
from sqlalchemy.dialects.postgresql import JSONB
from sqlalchemy.orm import Mapped, mapped_column

from app.database.base import Base


class Article(Base):
    """使用 slug 對應前台文章網址。"""

    __tablename__ = "articles"

    id:         Mapped[int] = mapped_column(Integer, primary_key=True)
    slug:       Mapped[str] = mapped_column(String(100), unique=True, index=True)
    title:      Mapped[str] = mapped_column(String(200), nullable=False)
    content:    Mapped[dict[str, Any]] = mapped_column(JSONB, nullable=False)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())
    updated_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now(), onupdate=func.now())
