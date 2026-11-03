"""註冊、登入與 session 的商業流程。"""

from datetime import datetime, timedelta, timezone

from app.config.settings import settings
from app.core.security.passwords import hash_password, verify_password
from app.core.security.sessions import create_session_token, hash_session_token
from app.database.models.user_model import User
from app.database.repositories import auth_repository
from app.schemas.auth_schema import CurrentUserResponse


class EmailAlreadyRegisteredError(Exception):
    """註冊 Email 已被使用。"""


def register_user(*, email: str, password: str) -> CurrentUserResponse:
    """註冊新使用者。"""
    user = auth_repository.create_local_user(email=email, password_hash=hash_password(password))
    if user is None:
        raise EmailAlreadyRegisteredError
    return _as_current_user(user)


def authenticate_user(*, email: str, password: str) -> CurrentUserResponse | None:
    """使用 Email/密碼驗證使用者。"""
    result = auth_repository.get_user_and_password_by_email(email)
    password_hash = result[1] if result is not None else None
    password_is_valid = verify_password(password, password_hash)
    if result is None or not password_is_valid:
        return None
    return _as_current_user(result[0])


def create_browser_session(user_id: int) -> str:
    """建立新的登入 Session，並回傳 Session Token。"""
    session_token = create_session_token()
    expires_at = datetime.now(timezone.utc) + timedelta(days=settings.auth_session_ttl_days)
    auth_repository.create_session(
        user_id=user_id,
        token_hash=hash_session_token(session_token),
        expires_at=expires_at,
    )
    return session_token


def get_current_user(session_token: str | None) -> CurrentUserResponse | None:
    if not session_token:
        return None
    user = auth_repository.get_user_for_session(
        token_hash=hash_session_token(session_token),
        now=datetime.now(timezone.utc),
    )
    return _as_current_user(user) if user is not None else None


def revoke_browser_session(session_token: str | None) -> None:
    if session_token:
        auth_repository.revoke_session(
            token_hash=hash_session_token(session_token),
            now=datetime.now(timezone.utc),
        )


def _as_current_user(user: User) -> CurrentUserResponse:
    return CurrentUserResponse.model_validate(user)
