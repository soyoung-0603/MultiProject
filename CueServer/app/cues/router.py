from typing import Annotated
from uuid import UUID

from fastapi import APIRouter, Depends, Response
from sqlalchemy.orm import Session

from app.cues import service
from app.cues.schemas import CueFields, CueList, CueOrder, CuePatch, CueRead, ShowDetail, ShowName, ShowRead
from app.database import get_session


router = APIRouter(prefix="/api/v1/shows", tags=["shows"])
Database = Annotated[Session, Depends(get_session)]


@router.get("", response_model=list[ShowRead])
def list_shows(session: Database):
    return service.list_shows(session)


@router.post("", response_model=ShowRead, status_code=201)
def create_show(data: ShowName, session: Database):
    return service.create_show(session, data.name)


@router.get("/{show_id}", response_model=ShowDetail)
def get_show(show_id: UUID, session: Database):
    return service.show_detail(session, service.get_show(session, show_id))


@router.patch("/{show_id}", response_model=ShowRead)
def rename_show(show_id: UUID, data: ShowName, session: Database):
    return service.rename_show(session, show_id, data.name)


@router.delete("/{show_id}", status_code=204)
def delete_show(show_id: UUID, session: Database):
    service.delete_show(session, show_id)


@router.post("/{show_id}/cues", response_model=CueRead, status_code=201)
def create_cue(show_id: UUID, data: CueFields, session: Database):
    return service.create_cue(session, show_id, data)


@router.get("/{show_id}/cues/{cue_id}", response_model=CueRead)
def get_cue(show_id: UUID, cue_id: UUID, session: Database):
    show = service.get_show(session, show_id)
    return service.read_cue(service.get_cue(session, show, cue_id))


@router.patch("/{show_id}/cues/{cue_id}", response_model=CueRead)
def update_cue(show_id: UUID, cue_id: UUID, data: CuePatch, session: Database):
    return service.update_cue(session, show_id, cue_id, data)


@router.delete("/{show_id}/cues/{cue_id}", status_code=204)
def delete_cue(show_id: UUID, cue_id: UUID, session: Database):
    service.delete_cue(session, show_id, cue_id)


@router.put("/{show_id}/cue-order", response_model=ShowDetail)
def reorder_cues(show_id: UUID, data: CueOrder, session: Database):
    return service.reorder_cues(session, show_id, data.cue_ids)


@router.get("/{show_id}/cue-list", response_model=CueList)
def download_cue_list(show_id: UUID, session: Database):
    show = service.get_show(session, show_id)
    revision = service.published_revision(session, show)
    return Response(revision.snapshot_json, media_type="application/json", headers={"Cache-Control": "no-store"})
