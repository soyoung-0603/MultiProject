from pathlib import Path

from fastapi import Request
from sqlalchemy import URL, create_engine, event
from sqlalchemy.orm import DeclarativeBase, Session


class Base(DeclarativeBase):
    pass


def build_engine(database: Path):
    database.parent.mkdir(parents=True, exist_ok=True)
    engine = create_engine(URL.create("sqlite", database=str(database)))

    @event.listens_for(engine, "connect")
    def enable_foreign_keys(connection, _):
        connection.execute("PRAGMA foreign_keys = ON")

    return engine


def get_session(request: Request):
    with Session(request.app.state.engine) as session:
        try:
            yield session
        except Exception:
            session.rollback()
            raise
