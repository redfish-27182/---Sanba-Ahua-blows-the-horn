"""Email/密碼註冊、登入、登出與目前使用者 API。"""

from fastapi import APIRouter, HTTPException, Request, Response, status

from app.config.settings import settings
from app.schemas.auth_schema import CurrentUserResponse, LoginRequest, RegisterRequest
from app.services.auth_service import (
    EmailAlreadyRegisteredError,
    authenticate_user,
    create_browser_session,
    get_current_user,
    register_user,
    revoke_browser_session,
)


router = APIRouter(prefix="/auth", tags=["auth"])


@router.post("/register", response_model=CurrentUserResponse, status_code=status.HTTP_201_CREATED)
def register(data: RegisterRequest, response: Response) -> CurrentUserResponse:
    """註冊新使用者。"""
    try:
        user = register_user(email=data.email, password=data.password)
    except EmailAlreadyRegisteredError as error:
        raise HTTPException(status_code=status.HTTP_409_CONFLICT, detail="此 Email 已經註冊") from error
    _set_session_cookie(response, create_browser_session(user.id))
    return user


@router.post("/login", response_model=CurrentUserResponse)
def login(data: LoginRequest, response: Response) -> CurrentUserResponse:
    """使用 Email/密碼登入。"""
    user = authenticate_user(email=data.email, password=data.password)
    if user is None:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="帳號或密碼錯誤")
    _set_session_cookie(response, create_browser_session(user.id))
    return user


@router.get("/me", response_model=CurrentUserResponse)
def read_current_user(request: Request) -> CurrentUserResponse:
    """取得目前登入的使用者資訊。"""
    user = get_current_user(request.cookies.get(settings.auth_session_cookie_name))
    if user is None:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="尚未登入")
    return user


@router.post("/logout", status_code=status.HTTP_204_NO_CONTENT)
def logout(request: Request) -> Response:
    """登出目前使用者。"""
    revoke_browser_session(request.cookies.get(settings.auth_session_cookie_name))
    response = Response(status_code=status.HTTP_204_NO_CONTENT)
    response.delete_cookie(key=settings.auth_session_cookie_name, path="/")
    return response


def _set_session_cookie(response: Response, session_token: str) -> None:
    """設定登入 Session 的 Cookie。"""
    response.set_cookie(
        key=settings.auth_session_cookie_name,
        value=session_token,
        max_age=settings.auth_session_ttl_seconds,
        httponly=True,
        secure=settings.auth_cookie_secure,
        samesite="lax",
        path="/",
    )
