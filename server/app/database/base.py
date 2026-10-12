"""所有 SQLAlchemy 資料表 model 共用的基底類別。"""

from sqlalchemy.orm import DeclarativeBase


class Base(DeclarativeBase):
    """之後新增資料表時都繼承這個類別。"""
