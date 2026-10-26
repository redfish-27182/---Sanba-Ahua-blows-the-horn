"""新增設備位置的英文國家與行政區欄位。"""

from alembic import op
import sqlalchemy as sa


revision = "0004_add_location_names"
down_revision = "0003_create_device_locations"
branch_labels = None
depends_on = None


def upgrade() -> None:
    """將英文國家、行政區與城市欄位加入 device_locations。"""

    op.add_column("device_locations", sa.Column("country_code", sa.String(length=10)))
    op.add_column("device_locations", sa.Column("country_name", sa.String(length=100)))
    op.add_column(
        "device_locations",
        sa.Column("admin_area_level_1", sa.String(length=150)),
    )
    op.add_column(
        "device_locations",
        sa.Column("admin_area_level_2", sa.String(length=150)),
    )
    op.add_column("device_locations", sa.Column("locality", sa.String(length=150)))


def downgrade() -> None:
    """移除設備位置的英文國家與行政區欄位。"""

    op.drop_column("device_locations", "locality")
    op.drop_column("device_locations", "admin_area_level_2")
    op.drop_column("device_locations", "admin_area_level_1")
    op.drop_column("device_locations", "country_name")
    op.drop_column("device_locations", "country_code")
