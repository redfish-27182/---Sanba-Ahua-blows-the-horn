"""保護登入與管理員路由的 FastAPI 相依性。"""

from typing import Annotated

from fastapi import Depends, HTTPException, Request, status

from app.config.settings import settings
from app.schemas.auth_schema import CurrentUserResponse
from app.services import auth_service


def require_current_user(request: Request) -> CurrentUserResponse:
    user = auth_service.get_current_user(request.cookies.get(settings.auth_session_cookie_name))
    if user is None:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="需要登入")
    return user


def require_admin(user: Annotated[CurrentUserResponse, Depends(require_current_user)]) -> CurrentUserResponse:
    if user.role != "admin":
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="需要管理員權限")
    return user
