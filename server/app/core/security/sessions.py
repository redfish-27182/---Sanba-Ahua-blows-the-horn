"""產生與雜湊瀏覽器 session token。"""

from hashlib import sha256
from secrets import token_urlsafe


def create_session_token() -> str:
    return token_urlsafe(32)


def hash_session_token(session_token: str) -> str:
    return sha256(session_token.encode("utf-8")).hexdigest()
