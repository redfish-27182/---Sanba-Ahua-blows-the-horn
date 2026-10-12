"""建立最小的設備狀態資料表。"""

from alembic import op
import sqlalchemy as sa


# Alembic 用這些值決定 migration 的先後順序。
revision = "0001_create_device_status"
down_revision = None
branch_labels = None
depends_on = None


def upgrade() -> None:
    """建立每台 ESP32 最後 online / offline 狀態的資料表。"""
    op.create_table(
        "device_status",
        sa.Column("id", sa.Integer(), primary_key=True),
        sa.Column("device_id", sa.String(length=100), nullable=False),
        sa.Column("status", sa.String(length=16), nullable=False),
        sa.Column(
            "updated_at",
            sa.DateTime(timezone=True),
            server_default=sa.text("now()"),
            nullable=False,
        ),
    )
    op.create_index("ix_device_status_device_id", "device_status", ["device_id"], unique=True)


def downgrade() -> None:
    """需要退回第一版前時，移除設備狀態資料表。"""
    op.drop_index("ix_device_status_device_id", table_name="device_status")
    op.drop_table("device_status")
