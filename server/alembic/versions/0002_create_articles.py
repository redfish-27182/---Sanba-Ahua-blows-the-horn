"""建立使用者指南文章資料表。"""
from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

revision = "0002_create_articles"
down_revision = "0001_create_device_status"
branch_labels = None
depends_on = None


def upgrade() -> None:
    """建立文章資料表及不重複的網址索引。"""
    op.create_table(
        "articles",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("slug", sa.String(100), nullable=False),
        sa.Column("title", sa.String(200), nullable=False),
        sa.Column("content", postgresql.JSONB(), nullable=False),
        sa.Column("created_at", sa.DateTime(timezone=True), nullable=False, server_default=sa.func.now()),
        sa.Column("updated_at", sa.DateTime(timezone=True), nullable=False, server_default=sa.func.now()),
    )
    op.create_index("ix_articles_slug", "articles", ["slug"], unique=True)


def downgrade() -> None:
    """退版時移除文章表；此操作會刪除文章資料。"""
    op.drop_index("ix_articles_slug", table_name="articles")
    op.drop_table("articles")
