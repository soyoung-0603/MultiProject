from fastapi.testclient import TestClient
import pytest

from app.main import create_app


FIELDS = {
    "name": "Stage Cue", "animationState": "Idle", "cameraState": "Stage",
    "lightingState": "Neutral", "appearancePreset": "Preset_01",
}


def create_show(client):
    response = client.post("/api/v1/shows", json={"name": "Stage compatibility"})
    assert response.status_code == 201
    return "/api/v1/shows/" + response.json()["id"]


@pytest.mark.parametrize("camera", ["Full", "Upper", "Close", "Stage"])
def test_camera_choices_create_edit_read_and_download(tmp_path, camera):
    with TestClient(create_app(tmp_path / "stage.sqlite3")) as client:
        show = create_show(client)
        response = client.post(show + "/cues", json=FIELDS | {"cameraState": camera})
        assert response.status_code == 201, response.text
        cue = response.json()
        assert cue["cameraState"] == camera
        assert cue["number"] == cue["position"] == 1
        assert cue["cameraBlendMs"] == cue["lightingFadeMs"] == 0
        url = show + "/cues/" + cue["id"]
        assert client.get(url).json() == cue
        edited = client.patch(url, json={"cameraState": "Stage"})
        assert edited.status_code == 200
        assert edited.json() == cue | {"cameraState": "Stage"}
        snapshot = client.get(show + "/cue-list").json()
        assert snapshot["schemaVersion"] == 2
        assert snapshot["cues"] == [edited.json()]
        assert client.get(show).json()["cues"] == snapshot["cues"]


def test_hidden_transition_values_survive_edit_reorder_and_restart(tmp_path):
    database = tmp_path / "legacy-times.sqlite3"
    with TestClient(create_app(database)) as client:
        show = create_show(client)
        created = client.post(show + "/cues", json=FIELDS | {
            "cameraState": "Upper", "cameraBlendMs": 1234, "lightingFadeMs": 5678,
        }).json()
        second = client.post(show + "/cues", json=FIELDS | {"name": "Second"}).json()
        url = show + "/cues/" + created["id"]
        edited = client.patch(url, json={"cameraState": "Stage"})
        assert edited.status_code == 200
        assert edited.json() == created | {"cameraState": "Stage"}
        reordered = client.put(show + "/cue-order", json={"cueIds": [second["id"], created["id"]]})
        assert reordered.status_code == 200
        snapshot = client.get(show + "/cue-list").json()
        assert snapshot["cues"][1] == created | {"cameraState": "Stage", "number": 2, "position": 2}
        assert [cue["number"] for cue in snapshot["cues"]] == [1, 2]
        baseline = client.get(show + "/cue-list").content
        assert client.patch(url, json={"cameraState": "stage"}).status_code == 422
        assert client.get(show + "/cue-list").content == baseline
    with TestClient(create_app(database)) as restarted:
        assert restarted.get(show + "/cue-list").json() == snapshot


def test_camera_openapi_preserves_legacy_transition_contract(tmp_path):
    with TestClient(create_app(tmp_path / "schema.sqlite3")) as client:
        schemas = client.get("/openapi.json").json()["components"]["schemas"]
        assert schemas["CueFields"]["properties"]["cameraState"]["enum"] == ["Full", "Upper", "Close", "Stage"]
        for field in ("cameraBlendMs", "lightingFadeMs"):
            assert field not in schemas["CueFields"]["required"]
            assert schemas["CueRead"]["properties"][field]["type"] == "integer"
            assert field in schemas["CueRead"]["required"]
