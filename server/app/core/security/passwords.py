"""密碼雜湊與驗證；原始密碼不會寫入資料庫。"""

from pwdlib import PasswordHash


password_hasher = PasswordHash.recommended()
_DUMMY_HASH = password_hasher.hash("not-a-real-password")


def hash_password(password: str) -> str:
    return password_hasher.hash(password)


def verify_password(password: str, password_hash: str | None) -> bool:
    # 找不到帳號時也驗證假雜湊，降低帳號是否存在的時間差異。
    return password_hasher.verify(password, password_hash or _DUMMY_HASH)
