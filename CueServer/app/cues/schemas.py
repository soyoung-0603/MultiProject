from datetime import datetime
from typing import Annotated, Literal
from uuid import UUID

from pydantic import BaseModel, ConfigDict, Field, StringConstraints, field_validator, model_validator
from pydantic.alias_generators import to_camel


MAX_CUES = 1000
MAX_REVISION = 2147483647
Name = Annotated[str, StringConstraints(strip_whitespace=True, min_length=1, max_length=100)]
Number = Annotated[int, Field(strict=True, ge=1, le=999999)]
Duration = Annotated[int, Field(strict=True, ge=0, le=30000)]
Position = Annotated[int, Field(strict=True, ge=1, le=MAX_CUES)]
RevisionNumber = Annotated[int, Field(strict=True, ge=1, le=MAX_REVISION)]
Animation = Literal["Idle", "Talk", "Dance"]
Camera = Literal["Full", "Upper", "Close"]
Lighting = Literal["Neutral", "Focus", "Stage"]
Preset = Literal["Preset_01", "Preset_02", "Preset_03", "Preset_04", "Preset_05"]


class Payload(BaseModel):
    model_config = ConfigDict(alias_generator=to_camel, extra="forbid")


class ShowName(Payload):
    name: Name


class ShowRead(ShowName):
    id: UUID
    revision: RevisionNumber
    created_at: datetime
    updated_at: datetime


class CueFields(Payload):
    number: Number
    name: Name
    animation_state: Animation
    camera_state: Camera
    lighting_state: Lighting
    appearance_preset: Preset
    camera_blend_ms: Duration
    lighting_fade_ms: Duration


class CuePatch(Payload):
    number: Number | None = None
    name: Name | None = None
    animation_state: Animation | None = None
    camera_state: Camera | None = None
    lighting_state: Lighting | None = None
    appearance_preset: Preset | None = None
    camera_blend_ms: Duration | None = None
    lighting_fade_ms: Duration | None = None

    @field_validator("*", mode="before")
    @classmethod
    def reject_null(cls, value):
        if value is None:
            raise ValueError("Supply a value or omit this field.")
        return value

    @model_validator(mode="after")
    def require_change(self):
        if not self.model_fields_set:
            raise ValueError("Supply at least one Cue field.")
        return self


class CueRead(CueFields):
    id: UUID
    position: Position


class CueOrder(Payload):
    cue_ids: Annotated[list[UUID], Field(max_length=MAX_CUES)]


class ShowDetail(ShowRead):
    cues: list[CueRead]


class CueList(Payload):
    schema_version: Literal[2]
    show_id: UUID
    show_name: Name
    revision: RevisionNumber
    published_at: datetime
    cues: Annotated[list[CueRead], Field(max_length=MAX_CUES)]
