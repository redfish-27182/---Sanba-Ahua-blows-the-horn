"""建立每台設備的最新天氣資料表。"""

from alembic import op
import sqlalchemy as sa


revision = "0005_create_device_weather"
down_revision = "0004_add_location_names"
branch_labels = None
depends_on = None


def upgrade() -> None:
    """建立 device_weather 資料表，每台設備只保留一筆最新天氣。"""

    op.create_table(
        "device_weather",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("device_id", sa.String(length=100), nullable=False),
        sa.Column("latitude", sa.Float(), nullable=False),
        sa.Column("longitude", sa.Float(), nullable=False),
        sa.Column("temperature_c", sa.Float(), nullable=False),
        sa.Column("relative_humidity", sa.Integer(), nullable=False),
        sa.Column("feels_like_c", sa.Float(), nullable=False),
        sa.Column("weather_code", sa.Integer(), nullable=False),
        sa.Column("weather_description", sa.String(length=100), nullable=False),
        sa.Column("precipitation_probability", sa.Integer(), nullable=False),
        sa.Column("observed_at", sa.DateTime(timezone=True), nullable=False),
        sa.Column("fetched_at", sa.DateTime(timezone=True), nullable=False),
        sa.Column("provider", sa.String(length=50), nullable=False),
        sa.Column(
            "updated_at",
            sa.DateTime(timezone=True),
            server_default=sa.text("now()"),
            nullable=False,
        ),
    )
    op.create_index(
        "ix_device_weather_device_id",
        "device_weather",
        ["device_id"],
        unique=True,
    )


def downgrade() -> None:
    """移除每台設備的最新天氣資料表。"""

    op.drop_index("ix_device_weather_device_id", table_name="device_weather")
    op.drop_table("device_weather")
