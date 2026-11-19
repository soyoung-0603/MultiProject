import json

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.cues.models import Show
from app.cues.schemas import CueList
from app.cues.service import assign_order, ordered_cues, publish
from app.database import Base


def initialize_database(engine):
    with engine.begin() as connection:
        # Keep the table change and every snapshot conversion in one SQLite transaction.
        connection.exec_driver_sql("BEGIN IMMEDIATE")
        Base.metadata.create_all(connection)
        columns = connection.exec_driver_sql("PRAGMA table_info(cues)")
        if "shader_mode" in {row.name for row in columns}:
            connection.exec_driver_sql("ALTER TABLE cues DROP COLUMN shader_mode")
            revisions = connection.exec_driver_sql("SELECT show_id, revision, snapshot_json FROM revisions").all()
            for show_id, revision, body in revisions:
                snapshot = json.loads(body)
                if snapshot["schemaVersion"] != 1:
                    raise ValueError(f"Unexpected snapshot version for {show_id} revision {revision}")
                snapshot["schemaVersion"] = 2
                for cue in snapshot["cues"]:
                    del cue["shaderMode"]
                CueList.model_validate(snapshot)
                connection.exec_driver_sql(
                    "UPDATE revisions SET snapshot_json = ? WHERE show_id = ? AND revision = ?",
                    (json.dumps(snapshot, ensure_ascii=False, separators=(",", ":")), show_id, revision),
                )

        # Publish the new numbering once per affected Show, keeping its old revisions intact.
        # The bound Session flushes into this same transaction; any failure rolls it all back.
        with Session(bind=connection) as session:
            for show in session.scalars(select(Show)).all():
                cues = ordered_cues(session, show)
                if any(cue.position != i or cue.number != i for i, cue in enumerate(cues, 1)):
                    assign_order(session, cues)
                    publish(session, show)
            session.flush()
