import json
import sqlite3

import pytest
from fastapi.testclient import TestClient
from sqlalchemy.exc import IntegrityError

from app.main import create_app
from app.cues.models import Cue
from app.cues import service
from sqlalchemy.orm import Session


FIELDS = {
    "name": "OPENING", "animationState": "Idle", "cameraState": "Full",
    "lightingState": "Neutral", "appearancePreset": "Preset_01",
    "cameraBlendMs": 1200, "lightingFadeMs": 700,
}


def show(client):
    result = client.post("/api/v1/shows", json={"name": "Existing production"})
    assert result.status_code == 201
    return "/api/v1/shows/" + result.json()["id"]


def add(client, url, **changes):
    result = client.post(url + "/cues", json=FIELDS | changes)
    assert result.status_code == 201, result.text
    return result.json()


def assert_order(client, url, originals):
    detail = client.get(url).json()
    snapshot = client.get(url + "/cue-list").json()
    assert snapshot["schemaVersion"] == 2
    assert snapshot["revision"] == detail["revision"]
    assert snapshot["cues"] == detail["cues"]
    for position, (actual, original) in enumerate(zip(snapshot["cues"], originals, strict=True), 1):
        assert actual == original | {"position": position, "number": position}
        assert type(actual["number"]) is int
        assert client.get(url + "/cues/" + actual["id"]).json() == actual
    return snapshot


def test_auto_number_create_reorder_delete_append_and_restart(tmp_path):
    db = tmp_path / "show.sqlite3"
    with TestClient(create_app(db)) as client:
        url = show(client)
        first = add(client, url)
        second = add(client, url, name="MAIN TALK", animationState="Talk", cameraState="Upper")
        third = add(client, url, name="PERFORMANCE", animationState="Dance", lightingState="Stage")
        assert_order(client, url, [first, second, third])
        result = client.put(url + "/cue-order", json={"cueIds": [third["id"], first["id"], second["id"]]})
        assert result.status_code == 200
        assert_order(client, url, [third, first, second])
        assert client.delete(url + "/cues/" + first["id"]).status_code == 204
        assert_order(client, url, [third, second])
        fourth = add(client, url, name="CLOSING")
        expected = assert_order(client, url, [third, second, fourth])
        other = show(client)
        assert add(client, other)["number"] == 1
    with TestClient(create_app(db)) as restarted:
        assert restarted.get(url + "/cue-list").json() == expected
        assert_order(restarted, url, [third, second, fourth])


def test_number_is_output_only_and_clone_gets_a_new_id(tmp_path):
    with TestClient(create_app(tmp_path / "test.sqlite3")) as client:
        url = show(client)
        first = add(client, url)
        for value in (1, 100, None):
            assert client.post(url + "/cues", json=FIELDS | {"number": value}).status_code == 422
            assert client.patch(url + "/cues/" + first["id"], json={"number": value}).status_code == 422
        cloned = add(client, url)
        assert cloned["id"] != first["id"]
        assert_order(client, url, [first, cloned])
        schemas = client.get("/openapi.json").json()["components"]["schemas"]
        assert "number" not in schemas["CueFields"]["properties"]
        assert "number" not in schemas["CuePatch"]["properties"]
        assert schemas["CueRead"]["properties"]["number"]["type"] == "integer"


@pytest.fixture
def old_number_db(tmp_path):
    db = tmp_path / "existing.sqlite3"
    with TestClient(create_app(db)) as client:
        url = show(client)
        cues = [add(client, url, name=name) for name in ("OPENING", "MAIN TALK", "PERFORMANCE")]
        assert client.put(url + "/cue-order", json={"cueIds": [cues[2]["id"], cues[0]["id"], cues[1]["id"]]}).status_code == 200
    with sqlite3.connect(db) as connection:
        rows = connection.execute("SELECT * FROM cues ORDER BY position").fetchall()
        # Simulate the previous editable-number policy with colliding target values.
        connection.execute("UPDATE cues SET number = -position")
        for row, number in zip(rows, (20, 1, 10), strict=True):
            connection.execute("UPDATE cues SET number=? WHERE id=?", (number, row[0]))
        body, = connection.execute("SELECT snapshot_json FROM revisions ORDER BY revision DESC LIMIT 1").fetchone()
        snapshot = json.loads(body)
        for cue, number in zip(snapshot["cues"], (20, 1, 10), strict=True):
            cue["number"] = number
        connection.execute("UPDATE revisions SET snapshot_json=? WHERE revision=?", (json.dumps(snapshot), snapshot["revision"]))
        original_rows = connection.execute("SELECT * FROM cues ORDER BY position").fetchall()
        history = connection.execute("SELECT * FROM revisions ORDER BY revision").fetchall()
    return db, url, snapshot, original_rows, history


def test_existing_data_migration_preserves_history_and_is_idempotent(old_number_db):
    db, url, previous, original_rows, history = old_number_db
    with TestClient(create_app(db)) as client:
        updated = assert_order(client, url, previous["cues"])
        assert updated["revision"] == previous["revision"] + 1
    with sqlite3.connect(db) as connection:
        actual_history = connection.execute("SELECT * FROM revisions ORDER BY revision").fetchall()
        assert actual_history[:-1] == history
        actual = connection.execute("SELECT * FROM cues ORDER BY position").fetchall()
        columns = [row[1] for row in connection.execute("PRAGMA table_info(cues)")]
        ignored = {columns.index("number"), columns.index("position")}
        assert [[v for i, v in enumerate(row) if i not in ignored] for row in actual] == [
            [v for i, v in enumerate(row) if i not in ignored] for row in original_rows]
    with TestClient(create_app(db)) as restarted:
        assert restarted.get(url + "/cue-list").json() == updated


def test_failed_number_migration_rolls_back_everything(old_number_db):
    db, _, _, rows, history = old_number_db
    with sqlite3.connect(db) as connection:
        connection.execute("CREATE TRIGGER fail_revision BEFORE INSERT ON revisions BEGIN SELECT RAISE(ABORT, 'migration test'); END")
    with pytest.raises(IntegrityError, match="migration test"), TestClient(create_app(db)):
        pytest.fail("Expected an atomic migration failure")
    with sqlite3.connect(db) as connection:
        assert connection.execute("SELECT * FROM cues ORDER BY position").fetchall() == rows
        assert connection.execute("SELECT * FROM revisions ORDER BY revision").fetchall() == history


@pytest.mark.parametrize("fail", [False, True])
def test_v1_conversion_and_number_migration_share_one_transaction(old_number_db, fail):
    db, url, previous, _, _ = old_number_db
    with sqlite3.connect(db) as connection:
        connection.execute("ALTER TABLE cues ADD COLUMN shader_mode TEXT NOT NULL DEFAULT 'ORIGINAL'")
        for revision, body in connection.execute("SELECT revision,snapshot_json FROM revisions").fetchall():
            snapshot = json.loads(body)
            snapshot["schemaVersion"] = 1
            for cue in snapshot["cues"]:
                cue["shaderMode"] = "TOON"
            connection.execute("UPDATE revisions SET snapshot_json=? WHERE revision=?", (json.dumps(snapshot), revision))
        history = connection.execute("SELECT * FROM revisions ORDER BY revision").fetchall()
        rows = connection.execute("SELECT * FROM cues ORDER BY position").fetchall()
        if fail:
            connection.execute("CREATE TRIGGER fail_new_revision BEFORE INSERT ON revisions BEGIN SELECT RAISE(ABORT, 'combined migration test'); END")
    if fail:
        with pytest.raises(IntegrityError, match="combined migration test"), TestClient(create_app(db)):
            pytest.fail("Expected a complete rollback")
        with sqlite3.connect(db) as connection:
            assert "shader_mode" in [row[1] for row in connection.execute("PRAGMA table_info(cues)")]
            assert connection.execute("SELECT * FROM cues ORDER BY position").fetchall() == rows
            assert connection.execute("SELECT * FROM revisions ORDER BY revision").fetchall() == history
        return
    with TestClient(create_app(db)) as client:
        updated = assert_order(client, url, previous["cues"])
        assert updated["revision"] == previous["revision"] + 1
    with sqlite3.connect(db) as connection:
        actual = connection.execute("SELECT * FROM revisions ORDER BY revision").fetchall()
        assert len(actual) == len(history) + 1
        for old, new in zip(history, actual[:-1], strict=True):
            assert old[:3] == new[:3] and old[4] == new[4]
            expected = json.loads(old[3])
            expected["schemaVersion"] = 2
            for cue in expected["cues"]:
                del cue["shaderMode"]
            assert json.loads(new[3]) == expected
    with TestClient(create_app(db)) as restarted:
        assert restarted.get(url + "/cue-list").json() == updated


def test_api_preserves_integer_number_1000(tmp_path):
    with TestClient(create_app(tmp_path / "large.sqlite3")) as client:
        url = show(client)
        show_id = url.rsplit("/", 1)[1]
        with Session(client.app.state.engine) as session, service.write(session):
            current = service.get_show(session, show_id)
            from app.cues.schemas import CueFields
            values = CueFields.model_validate(FIELDS).model_dump()
            from uuid import uuid4
            session.add_all([Cue(id=str(uuid4()), show_id=show_id, position=i, number=i, **values) for i in range(1, 1001)])
            service.publish(session, current)
        cues = client.get(url + "/cue-list").json()["cues"]
        assert [cue["number"] for cue in cues] == list(range(1, 1001))
        assert type(cues[-1]["number"]) is int
        assert client.post(url + "/cues", json=FIELDS).status_code == 422
