from sqlalchemy import ForeignKey, Text, UniqueConstraint
from sqlalchemy.orm import Mapped, mapped_column

from app.database import Base


class Show(Base):
    __tablename__ = "shows"

    id: Mapped[str] = mapped_column(primary_key=True)
    name: Mapped[str]
    current_revision: Mapped[int]
    created_at: Mapped[str]
    updated_at: Mapped[str]
    deleted_at: Mapped[str | None]


class Cue(Base):
    __tablename__ = "cues"
    __table_args__ = (
        UniqueConstraint("show_id", "position"),
        UniqueConstraint("show_id", "number"),
    )

    id: Mapped[str] = mapped_column(primary_key=True)
    show_id: Mapped[str] = mapped_column(ForeignKey("shows.id"))
    position: Mapped[int]
    number: Mapped[int]
    name: Mapped[str]
    animation_state: Mapped[str]
    camera_state: Mapped[str]
    lighting_state: Mapped[str]
    appearance_preset: Mapped[str]
    camera_blend_ms: Mapped[int]
    lighting_fade_ms: Mapped[int]


class Revision(Base):
    __tablename__ = "revisions"

    show_id: Mapped[str] = mapped_column(ForeignKey("shows.id"), primary_key=True)
    revision: Mapped[int] = mapped_column(primary_key=True)
    created_at: Mapped[str]
    snapshot_json: Mapped[str] = mapped_column(Text)
    is_deleted: Mapped[bool]
