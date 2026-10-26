"""SQLite v4 end-to-end local HTTP integration."""

from __future__ import annotations

import http.client
import json
import threading
from pathlib import Path
from typing import Any

import pytest

from boardlab.games.dado_goal_http import (
    create_lifecycle_local_server,
)
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy


@pytest.fixture
def server(tmp_path: Path) -> Any:
    instance = create_lifecycle_local_server(
        tmp_path / "http-v4.sqlite",
        LifecyclePolicy(60, 120, 300),
    )
    thread = threading.Thread(
        target=instance.serve_forever,
        daemon=True,
    )
    thread.start()
    yield instance
    instance.shutdown()
    instance.server_close()
    thread.join(timeout=5)


def request(
    server: Any,
    method: str,
    path: str,
    body: dict[str, Any] | None = None,
    token: str | None = None,
) -> tuple[int, dict[str, Any]]:
    connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=5)
    headers = {
        "Host": f"127.0.0.1:{server.server_port}",
    }
    payload = None
    if body is not None:
        payload = json.dumps(body)
        headers["Content-Type"] = "application/json"
    if token is not None:
        headers["Authorization"] = f"Bearer {token}"

    try:
        connection.request(method, path, payload, headers)
        response = connection.getresponse()
        data = json.loads(response.read())
        return response.status, data
    finally:
        connection.close()


def test_v4_http_create_join_and_authenticated_snapshot(
    server: Any,
) -> None:
    status, created = request(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    assert status == 201

    match_id = created["matchId"]
    status, joined = request(
        server,
        "POST",
        f"/api/v1/matches/{match_id}/join",
        {"inviteToken": created["inviteToken"]},
    )
    assert status == 200

    status, snapshot = request(
        server,
        "GET",
        f"/api/v1/matches/{match_id}",
        token=joined["guestToken"],
    )
    assert status == 200
    assert snapshot["status"] == "active"
    assert snapshot["seat"] == 1


def test_v4_http_roll_and_replay(server: Any) -> None:
    _, created = request(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    match_id = created["matchId"]
    request(
        server,
        "POST",
        f"/api/v1/matches/{match_id}/join",
        {"inviteToken": created["inviteToken"]},
    )

    body = {"requestId": "http-v4-1", "expectedRevision": 1}
    path = f"/api/v1/matches/{match_id}/roll"

    status, result = request(server, "POST", path, body, created["hostToken"])
    assert status == 200
    assert result["revision"] == 2

    status, replay = request(server, "POST", path, body, created["hostToken"])
    assert status == 200
    assert replay["duplicate"] is True
    assert replay["face"] == result["face"]


def test_v4_http_rejects_wrong_credential(server: Any) -> None:
    _, created = request(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    status, payload = request(
        server,
        "GET",
        f"/api/v1/matches/{created['matchId']}",
        token=created["inviteToken"],
    )
    assert status == 401
    assert payload["error"]["code"] == "UNAUTHORIZED"
