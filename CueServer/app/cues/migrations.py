import json

from app.cues.schemas import CueList
from app.database import Base


def initialize_database(engine):
    with engine.begin() as connection:
        # Keep the table change and every snapshot conversion in one SQLite transaction.
        connection.exec_driver_sql("BEGIN IMMEDIATE")
        Base.metadata.create_all(connection)
        columns = connection.exec_driver_sql("PRAGMA table_info(cues)")
        if "shader_mode" not in {row.name for row in columns}:
            return

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
