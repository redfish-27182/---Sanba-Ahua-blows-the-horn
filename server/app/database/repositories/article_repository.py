"""使用 SQLAlchemy ORM 存取文章，不在這裡處理 HTTP。"""
from sqlalchemy import select

from app.database.models.article import Article
from app.database.session import SessionLocal
from app.schemas.article import ArticleInput, ArticleResponse


def get_article(slug: str) -> ArticleResponse | None:
    """依文章網址名稱查詢。"""
    with SessionLocal() as session:
        article = session.scalar(select(Article).where(Article.slug == slug))
        return ArticleResponse.model_validate(article) if article else None


def save_article(slug: str, data: ArticleInput) -> ArticleResponse:
    """建立文章，或更新相同 slug 的文章。"""
    with SessionLocal.begin() as session:
        article = session.scalar(select(Article).where(Article.slug == slug))
        if article is None:
            article = Article(slug=slug, title=data.title, content=data.content)
            session.add(article)
        else:
            article.title = data.title
            article.content = data.content
        session.flush()
        session.refresh(article)
        return ArticleResponse.model_validate(article)
