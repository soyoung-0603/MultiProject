from contextlib import contextmanager
from datetime import datetime, timezone
from uuid import uuid4

from sqlalchemy import select, text

from app.cues.models import Cue, Revision, Show
from app.cues.schemas import MAX_CUES, MAX_REVISION, CueList, CueRead, ShowDetail, ShowRead


class CueError(Exception):
    def __init__(self, status, code, message, field=None):
        super().__init__(message)
        self.status = status
        self.code = code
        self.field_errors = [{"field": field, "message": message}] if field else []


@contextmanager
def write(session):
    # Serialize SQLite writers before reading the state used to publish a revision.
    session.execute(text("BEGIN IMMEDIATE"))
    yield
    session.commit()


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def get_show(session, show_id):
    show = session.get(Show, str(show_id))
    if show is None or show.deleted_at is not None:
        raise CueError(404, "show_not_found", "Show를 찾을 수 없습니다.")
    return show


def get_cue(session, show, cue_id):
    cue = session.get(Cue, str(cue_id))
    if cue is None or cue.show_id != show.id:
        raise CueError(404, "cue_not_found", "이 Show에서 Cue를 찾을 수 없습니다.")
    return cue


def ordered_cues(session, show):
    return session.scalars(select(Cue).where(Cue.show_id == show.id).order_by(Cue.position)).all()


def read_cue(cue):
    return CueRead.model_validate(cue, from_attributes=True, by_name=True)


def read_show(show):
    return ShowRead(
        id=show.id, name=show.name, revision=show.current_revision,
        createdAt=show.created_at, updatedAt=show.updated_at,
    )


def list_shows(session):
    shows = session.scalars(select(Show).where(Show.deleted_at.is_(None)).order_by(Show.created_at, Show.id))
    return [read_show(show) for show in shows]


def published_revision(session, show):
    return session.get(Revision, (show.id, show.current_revision))


def show_detail(session, show):
    snapshot = CueList.model_validate_json(published_revision(session, show).snapshot_json)
    return ShowDetail(**read_show(show).model_dump(by_alias=True), cues=snapshot.cues)


def publish(session, show):
    if show.current_revision >= MAX_REVISION:
        raise CueError(503, "revision_limit", "이 Show의 Revision 한도에 도달했습니다.")
    show.current_revision += 1
    show.updated_at = utc_now()
    snapshot = CueList(
        schemaVersion=2, showId=show.id, showName=show.name, revision=show.current_revision,
        publishedAt=show.updated_at, cues=[read_cue(cue) for cue in ordered_cues(session, show)],
    )
    session.add(Revision(
        show_id=show.id, revision=show.current_revision, created_at=show.updated_at,
        snapshot_json=snapshot.model_dump_json(by_alias=True), is_deleted=show.deleted_at is not None,
    ))
    return snapshot


def create_show(session, name):
    with write(session):
        now = utc_now()
        show = Show(id=str(uuid4()), name=name, current_revision=0, created_at=now, updated_at=now)
        session.add(show)
        publish(session, show)
        return read_show(show)


def rename_show(session, show_id, name):
    with write(session):
        show = get_show(session, show_id)
        show.name = name
        publish(session, show)
        return read_show(show)


def delete_show(session, show_id):
    with write(session):
        show = get_show(session, show_id)
        show.deleted_at = utc_now()
        publish(session, show)


def check_number(session, show, number, current_id=None):
    existing_id = session.scalar(select(Cue.id).where(Cue.show_id == show.id, Cue.number == number))
    if existing_id is not None and existing_id != current_id:
        raise CueError(409, "duplicate_number", "이 번호는 사용 중입니다. 다른 번호를 입력하세요.", "number")


def create_cue(session, show_id, data):
    with write(session):
        show = get_show(session, show_id)
        position = len(ordered_cues(session, show)) + 1
        if position > MAX_CUES:
            raise CueError(422, "cue_limit", "Show당 Cue는 최대 1,000개입니다.")
        check_number(session, show, data.number)
        cue = Cue(id=str(uuid4()), show_id=show.id, position=position, **data.model_dump())
        session.add(cue)
        publish(session, show)
        return read_cue(cue)


def update_cue(session, show_id, cue_id, data):
    with write(session):
        show = get_show(session, show_id)
        cue = get_cue(session, show, cue_id)
        changes = data.model_dump(exclude_unset=True)
        if "number" in changes:
            check_number(session, show, changes["number"], cue.id)
        for field, value in changes.items():
            setattr(cue, field, value)
        publish(session, show)
        return read_cue(cue)


def assign_order(session, cues):
    # Temporary negative positions keep the unique constraint valid during swaps.
    moved = [(position, cue) for position, cue in enumerate(cues, 1) if cue.position != position]
    for position, cue in moved:
        cue.position = -position
    session.flush()
    for position, cue in moved:
        cue.position = position


def delete_cue(session, show_id, cue_id):
    with write(session):
        show = get_show(session, show_id)
        session.delete(get_cue(session, show, cue_id))
        assign_order(session, ordered_cues(session, show))
        publish(session, show)


def reorder_cues(session, show_id, cue_ids):
    with write(session):
        show = get_show(session, show_id)
        by_id = {cue.id: cue for cue in ordered_cues(session, show)}
        ids = [str(cue_id) for cue_id in cue_ids]
        if len(ids) != len(by_id) or set(ids) != by_id.keys():
            raise CueError(422, "invalid_order", "이 Show의 Cue를 각각 한 번씩 포함하세요.", "cueIds")
        assign_order(session, [by_id[cue_id] for cue_id in ids])
        snapshot = publish(session, show)
        return ShowDetail(**read_show(show).model_dump(by_alias=True), cues=snapshot.cues)
