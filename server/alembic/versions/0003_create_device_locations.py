"""建立設備最新定位資料表。"""

from alembic import op
import sqlalchemy as sa


revision = "0003_create_device_locations"
down_revision = "0002_create_articles"
branch_labels = None
depends_on = None


def upgrade() -> None:
    """建立每台設備只保留一筆最新位置的資料表。"""

    op.create_table(
        "device_locations",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("device_id", sa.String(length=100), nullable=False),
        sa.Column("latitude", sa.Float(), nullable=False),
        sa.Column("longitude", sa.Float(), nullable=False),
        sa.Column("accuracy_meters", sa.Float(), nullable=False),
        sa.Column("provider", sa.String(length=50), nullable=False),
        sa.Column("located_at", sa.DateTime(timezone=True), nullable=False),
        sa.Column(
            "updated_at",
            sa.DateTime(timezone=True),
            server_default=sa.text("now()"),
            nullable=False,
        ),
    )
    op.create_index(
        "ix_device_locations_device_id",
        "device_locations",
        ["device_id"],
        unique=True,
    )


def downgrade() -> None:
    """移除設備最新定位資料表。"""

    op.drop_index("ix_device_locations_device_id", table_name="device_locations")
    op.drop_table("device_locations")
