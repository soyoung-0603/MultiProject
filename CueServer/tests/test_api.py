import json
import sqlite3
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from uuid import UUID, uuid4

import pytest
from fastapi.testclient import TestClient
from sqlalchemy.exc import IntegrityError

from app.main import create_app


SHOWS = "/api/v1/shows"
CUE_FIELDS = {
    "number": 10,
    "name": "Standby",
    "animationState": "Idle",
    "cameraState": "Full",
    "lightingState": "Neutral",
    "appearancePreset": "Preset_01",
    "cameraBlendMs": 0,
    "lightingFadeMs": 0,
}


@pytest.fixture
def client(tmp_path):
    with TestClient(create_app(tmp_path / "cues.sqlite3")) as client:
        yield client


def create_show(client, name="Show"):
    response = client.post(SHOWS, json={"name": name})
    assert response.status_code == 201
    show = response.json()
    assert show["revision"] == 1
    UUID(show["id"])
    return f"{SHOWS}/{show['id']}"


def create_cue(client, show_url, **changes):
    response = client.post(f"{show_url}/cues", json=CUE_FIELDS | changes)
    assert response.status_code == 201
    return response.json()


def test_snapshot_survives_reopen(tmp_path):
    db = tmp_path / "cues.sqlite3"
    with TestClient(create_app(db)) as client:
        show = create_show(client, "Studio Demo")
        first = create_cue(client, show, number=10)
        second = create_cue(client, show, number=20, appearancePreset="Preset_02", cameraBlendMs=1500)
        third = create_cue(client, show, number=30, animationState="Dance", lightingFadeMs=30000)
        order = [second["id"], third["id"], first["id"]]
        response = client.put(f"{show}/cue-order", json={"cueIds": order})
        assert response.status_code == 200
        assert response.json()["revision"] == 5
        download = client.get(f"{show}/cue-list")
        assert download.status_code == 200
        snapshot = download.json()
        assert set(snapshot) == {"schemaVersion", "showId", "showName", "revision", "publishedAt", "cues"}
        assert snapshot["schemaVersion"] == 2
        assert snapshot["showId"] == show.rsplit("/", 1)[1]
        assert snapshot["showName"] == "Studio Demo"
        assert snapshot["revision"] == 5
        assert datetime.fromisoformat(snapshot["publishedAt"]).utcoffset().total_seconds() == 0
        assert snapshot["cues"] == [
            cue | {"position": position} for position, cue in enumerate((second, third, first), 1)
        ]
        assert set(first) == {*CUE_FIELDS, "position", "id"}
        UUID(first["id"])

    with TestClient(create_app(db)) as restarted:
        assert restarted.get(f"{show}/cue-list").content == download.content
        assert restarted.get(show).json()["cues"] == snapshot["cues"]
        assert restarted.get(SHOWS).json()[0]["revision"] == 5


def test_show_and_cue_crud_with_tombstone(client):
    assert client.get(SHOWS).json() == []
    show = create_show(client)
    assert client.get(f"{show}/cue-list").json()["cues"] == []
    assert client.patch(show, json={"name": "Rehearsal"}).json()["revision"] == 2
    first = create_cue(client, show)
    second = create_cue(client, show, number=20)
    first_url = f"{show}/cues/{first['id']}"
    second_url = f"{show}/cues/{second['id']}"

    assert client.get(first_url).json() == first
    edited = client.patch(first_url, json={"name": "Edited", "lightingFadeMs": 800})
    assert edited.status_code == 200
    assert edited.json() == first | {"name": "Edited", "lightingFadeMs": 800}
    assert client.delete(first_url).status_code == 204
    assert client.get(first_url).status_code == 404
    assert client.get(second_url).json() == second | {"position": 1}
    published = client.get(f"{show}/cue-list").json()
    assert published["revision"] == 6
    assert published["showName"] == "Rehearsal"
    assert client.delete(show).status_code == 204
    assert client.get(SHOWS).json() == []
    for path in (show, second_url, f"{show}/cue-list"):
        assert client.get(path).status_code == 404

    with client.app.state.engine.connect() as connection:
        rows = connection.exec_driver_sql(
            "SELECT revision, snapshot_json, is_deleted FROM revisions ORDER BY revision"
        ).all()
    assert [row.revision for row in rows] == list(range(1, 8))
    assert json.loads(rows[5].snapshot_json) == published
    assert rows[6].is_deleted == 1
    assert json.loads(rows[6].snapshot_json)["revision"] == 7


@pytest.mark.parametrize(
    ("field", "value"),
    [("number", 0), ("number", 1000000), ("number", True), ("number", "10"),
     ("name", "  "), ("name", "x" * 101), ("animationState", "Run"),
     ("cameraState", "Wide"), ("lightingState", "Dim"), ("shaderMode", "TOON"),
     ("appearancePreset", "Preset_06"), ("cameraBlendMs", -1), ("lightingFadeMs", 30001),
     ("lightingFadeMs", 1.5), ("lightingFadeMs", None), ("position", 1)],
)
def test_invalid_fields_leave_revision_unchanged(client, field, value):
    show = create_show(client)
    response = client.post(f"{show}/cues", json=CUE_FIELDS | {field: value})
    assert response.status_code == 422
    error = response.json()
    assert set(error) == {"code", "message", "fieldErrors"}
    assert error["code"] == "invalid_cue"
    assert error["fieldErrors"][0]["field"] == field
    assert client.get(f"{show}/cue-list").json()["revision"] == 1


def test_invalid_requests_and_number_conflict(client):
    assert client.post(SHOWS, json={"name": "  "}).status_code == 422
    assert client.get(f"{SHOWS}/not-a-uuid").status_code == 422
    assert client.get(f"{SHOWS}/{uuid4()}").status_code == 404
    show = create_show(client)
    first = create_cue(client, show)
    second = create_cue(client, show, number=20)
    other = create_cue(client, create_show(client))
    url = f"{show}/cues/{second['id']}"
    baseline = client.get(f"{show}/cue-list").content
    for response in (client.post(f"{show}/cues", json=CUE_FIELDS), client.patch(url, json={"number": 10})):
        assert response.status_code == 409
        assert response.json()["fieldErrors"][0]["field"] == "number"
    assert client.patch(url, json={"name": None}).status_code == 422
    assert client.patch(url, json={}).status_code == 422
    removed_field = client.patch(url, json={"name": "Ignored", "shaderMode": "ORIGINAL"})
    assert removed_field.status_code == 422
    assert removed_field.json()["fieldErrors"][0]["field"] == "shaderMode"
    assert client.post(f"{show}/cues", content="{").status_code == 422
    assert client.get(f"{show}/cues/{other['id']}").status_code == 404
    for ids in ([first["id"]], [first["id"], first["id"]], [first["id"], other["id"]]):
        response = client.put(f"{show}/cue-order", json={"cueIds": ids})
        assert response.status_code == 422
        assert response.json()["fieldErrors"][0]["field"] == "cueIds"
    assert client.get(f"{show}/cue-list").content == baseline


def test_revision_write_failure_rolls_back_changes(client):
    show = create_show(client)
    first = create_cue(client, show)
    second = create_cue(client, show, number=20)
    baseline = client.get(f"{show}/cue-list").content
    with client.app.state.engine.begin() as connection:
        assert connection.exec_driver_sql("PRAGMA foreign_keys").scalar_one() == 1
        connection.exec_driver_sql("""
            CREATE TRIGGER prevent_revision BEFORE INSERT ON revisions
            BEGIN SELECT RAISE(ABORT, 'test write failure'); END
        """)
    failed = client.put(f"{show}/cue-order", json={"cueIds": [second["id"], first["id"]]})
    assert failed.status_code == 503
    assert failed.json()["code"] == "storage_unavailable"
    assert client.get(f"{show}/cue-list").content == baseline
    assert client.get(f"{show}/cues/{first['id']}").json() == first
    assert client.get(show).json()["revision"] == 3
    with client.app.state.engine.begin() as connection:
        connection.exec_driver_sql("DROP TRIGGER prevent_revision")
    assert client.put(f"{show}/cue-order", json={"cueIds": [second["id"], first["id"]]}).status_code == 200


def test_simultaneous_writes_publish_complete_revisions(client):
    show = create_show(client)

    def submit(number):
        return client.post(f"{show}/cues", json=CUE_FIELDS | {"number": number}).status_code

    with ThreadPoolExecutor(max_workers=3) as pool:
        assert sorted(pool.map(submit, (10, 20, 10))) == [201, 201, 409]
    snapshot = client.get(f"{show}/cue-list").json()
    assert snapshot["revision"] == 3
    assert sorted(cue["number"] for cue in snapshot["cues"]) == [10, 20]
    assert [cue["position"] for cue in snapshot["cues"]] == [1, 2]


def test_same_origin_admin_and_openapi(client):
    for path in ("/", "/admin"):
        response = client.get(path)
        assert response.status_code == 200
        assert "text/html" in response.headers["content-type"]
        assert "Cue Inspector" in response.text
        assert "shaderMode" not in response.text
    js = client.get("/static/admin.js")
    assert js.status_code == 200
    assert "shaderMode" not in js.text
    assert client.get("/static/admin.css").status_code == 200
    openapi = client.get("/openapi.json").json()
    assert f"{SHOWS}/{{show_id}}/cue-list" in openapi["paths"]
    schemas = openapi["components"]["schemas"]
    for model in ("CueFields", "CuePatch", "CueRead"):
        assert "shaderMode" not in schemas[model]["properties"]
    assert schemas["CueList"]["properties"]["schemaVersion"]["const"] == 2


def read_history(connection):
    return connection.execute("SELECT * FROM revisions ORDER BY show_id, revision").fetchall()


@pytest.fixture
def legacy_db(tmp_path):
    db = tmp_path / "legacy.sqlite3"
    with TestClient(create_app(db)) as client:
        active = create_show(client, "Active")
        first = create_cue(client, active)
        second = create_cue(client, active, number=20, appearancePreset="Preset_02")
        assert client.put(f"{active}/cue-order", json={"cueIds": [second["id"], first["id"]]}).status_code == 200
        deleted = create_show(client, "Deleted")
        create_cue(client, deleted)
        assert client.delete(deleted).status_code == 204

    with sqlite3.connect(db) as connection:
        connection.execute("ALTER TABLE cues ADD COLUMN shader_mode TEXT NOT NULL DEFAULT 'ORIGINAL'")
        connection.execute("UPDATE cues SET shader_mode = 'TOON' WHERE number = 20")
        for show_id, revision, _, body, _ in read_history(connection):
            snapshot = json.loads(body)
            snapshot["schemaVersion"] = 1
            for cue in snapshot["cues"]:
                cue["shaderMode"] = "TOON" if cue["number"] == 20 else "ORIGINAL"
            connection.execute("UPDATE revisions SET snapshot_json = ? WHERE show_id = ? AND revision = ?",
                               (json.dumps(snapshot), show_id, revision))
        history = read_history(connection)
    return db, active, deleted, history


def test_upgrade_preserves_active_and_deleted_history(legacy_db):
    db, active, deleted, history = legacy_db
    with TestClient(create_app(db)) as client:
        migrated = client.get(f"{active}/cue-list").json()
        assert migrated["schemaVersion"] == 2
        assert migrated["revision"] == 4
        assert [cue["number"] for cue in migrated["cues"]] == [20, 10]
        assert client.get(active).json()["cues"] == migrated["cues"]
        assert client.get(f"{deleted}/cue-list").status_code == 404

    with sqlite3.connect(db) as connection:
        assert "shader_mode" not in [row[1] for row in connection.execute("PRAGMA table_info(cues)")]
        actual_history = read_history(connection)
        assert len(actual_history) == len(history)
        for old, new in zip(history, actual_history, strict=True):
            assert old[:3] == new[:3]
            assert old[4] == new[4]
            expected = json.loads(old[3])
            expected["schemaVersion"] = 2
            for cue in expected["cues"]:
                del cue["shaderMode"]
            assert json.loads(new[3]) == expected

    with TestClient(create_app(db)) as restarted:
        assert restarted.get(f"{active}/cue-list").json() == migrated
        assert create_cue(restarted, active, number=30)["position"] == 3
        assert restarted.get(active).json()["revision"] == 5


def test_failed_upgrade_leaves_legacy_database_intact(legacy_db):
    db, _, _, history = legacy_db
    with sqlite3.connect(db) as connection:
        connection.execute("""
            CREATE TRIGGER block_migration BEFORE UPDATE OF snapshot_json ON revisions
            WHEN OLD.revision = 2 BEGIN SELECT RAISE(ABORT, 'test migration failure'); END
        """)
    with pytest.raises(IntegrityError, match="test migration failure"), TestClient(create_app(db)):
        pytest.fail("Migration should fail at server startup")

    with sqlite3.connect(db) as connection:
        assert "shader_mode" in [row[1] for row in connection.execute("PRAGMA table_info(cues)")]
        assert read_history(connection) == history
