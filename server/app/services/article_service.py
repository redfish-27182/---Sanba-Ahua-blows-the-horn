"""文章邏輯；資料庫操作交由 repository 執行。"""
from app.database.repositories import article_repository
from app.schemas.article import ArticleInput, ArticleResponse


def get_article(slug: str) -> ArticleResponse | None:
    """依網址名稱取得文章。"""
    return article_repository.get_article(slug)


def save_article(slug: str, data: ArticleInput) -> ArticleResponse:
    """儲存文章，現有 slug 對應的文章會更新。"""
    return article_repository.save_article(slug, data)
