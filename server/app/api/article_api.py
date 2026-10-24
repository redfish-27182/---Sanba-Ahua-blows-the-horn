"""文章 HTTP 路由；寫入只供明確開啟的本機測試。"""
from ipaddress import ip_address
from typing import Annotated

from fastapi import APIRouter, HTTPException, Path, Request

from app.config.settings import LOCAL_ORIGINS, settings
from app.schemas.article_schema import ArticleInput, ArticleResponse
from app.services import article_service

router = APIRouter(prefix="/api/articles", tags=["articles"])
Slug = Annotated[str, Path(min_length=1, max_length=100, pattern=r"^[a-z0-9]+(?:-[a-z0-9]+)*$")]


@router.get("/{slug}", response_model=ArticleResponse)
def get_article(slug: Slug):
    """讀取一篇文章；找不到時回傳 404。"""
    article = article_service.get_article(slug)
    if article is None:
        raise HTTPException(status_code=404, detail="找不到文章")
    return article


@router.put("/{slug}", response_model=ArticleResponse)
def put_article(slug: Slug, data: ArticleInput, request: Request):
    """儲存文章；未實作身分驗證，不得用於公開部署。"""
    if not settings.article_edit_enabled:
        raise HTTPException(status_code=403, detail="文章寫入功能未啟用")
    try:
        is_local = request.client is not None and ip_address(request.client.host).is_loopback
    except ValueError:
        is_local = False
    origin = request.headers.get("origin")
    if not is_local or (origin is not None and origin not in LOCAL_ORIGINS):
        raise HTTPException(status_code=403, detail="暫時只允許本機編輯")
    return article_service.save_article(slug, data)
