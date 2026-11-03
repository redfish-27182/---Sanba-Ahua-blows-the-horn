"""使用者、密碼憑證與 session 的資料庫存取。"""

from datetime import datetime

from sqlalchemy import select

from app.database.models.auth_session_model import AuthSession
from app.database.models.user_model import User, UserPasswordCredential
from app.database.session import SessionLocal


def create_local_user(*, email: str, password_hash: str) -> User | None:
    with SessionLocal.begin() as session:
        existing_user = session.scalar(select(User).where(User.email == email))
        if existing_user is not None:
            return None

        user = User(email=email, role="user")
        session.add(user)
        session.flush()
        session.add(UserPasswordCredential(user_id=user.id, password_hash=password_hash))
        session.flush()
        session.refresh(user)
        return user


def get_user_and_password_by_email(email: str) -> tuple[User, str] | None:
    with SessionLocal() as session:
        result = session.execute(
            select(User, UserPasswordCredential.password_hash)
            .join(UserPasswordCredential, UserPasswordCredential.user_id == User.id)
            .where(User.email == email, User.is_active.is_(True))
        ).one_or_none()
        if result is None:
            return None
        return result[0], result[1]


def create_session(*, user_id: int, token_hash: str, expires_at: datetime) -> None:
    with SessionLocal.begin() as session:
        session.add(AuthSession(user_id=user_id, token_hash=token_hash, expires_at=expires_at))


def get_user_for_session(*, token_hash: str, now: datetime) -> User | None:
    with SessionLocal() as session:
        return session.scalar(
            select(User)
            .join(AuthSession, AuthSession.user_id == User.id)
            .where(
                AuthSession.token_hash == token_hash,
                AuthSession.revoked_at.is_(None),
                AuthSession.expires_at > now,
                User.is_active.is_(True),
            )
        )


def revoke_session(*, token_hash: str, now: datetime) -> None:
    with SessionLocal.begin() as session:
        auth_session = session.scalar(select(AuthSession).where(AuthSession.token_hash == token_hash))
        if auth_session is not None and auth_session.revoked_at is None:
            auth_session.revoked_at = now
