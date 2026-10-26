"""HTTP lifecycle recovery and expiration qualification for SQLite v4."""

from __future__ import annotations

import http.client
import json
import threading
from pathlib import Path
from typing import Any

from boardlab.games.dado_goal_http import create_local_server
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import LifecycleAuthorityV4


class Clock:
    def __init__(self, value: int = 1000) -> None:
        self.value = value

    def __call__(self) -> int:
        return self.value


class RunningServer:
    def __init__(self, manager: LifecycleAuthorityV4) -> None:
        self.server = create_local_server(manager)
        self.thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )

    def __enter__(self) -> RunningServer:
        self.thread.start()
        return self

    def __exit__(self, *_: object) -> None:
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)

    def request(
        self,
        method: str,
        path: str,
        body: dict[str, Any] | None = None,
        token: str | None = None,
    ) -> tuple[int, dict[str, Any]]:
        connection = http.client.HTTPConnection("127.0.0.1", self.server.server_port, timeout=5)
        headers = {"Host": f"127.0.0.1:{self.server.server_port}"}
        payload = None
        if body is not None:
            headers["Content-Type"] = "application/json"
            payload = json.dumps(body)
        if token is not None:
            headers["Authorization"] = f"Bearer {token}"
        try:
            connection.request(method, path, payload, headers)
            response = connection.getresponse()
            return response.status, json.loads(response.read())
        finally:
            connection.close()


def setup_manager(path: Path, clock: Clock) -> LifecycleAuthorityV4:
    manager = LifecycleAuthorityV4(path, LifecyclePolicy(60, 120, 300), clock=clock)
    manager.initialize()
    return manager


def test_http_restart_recovers_both_seats(tmp_path: Path) -> None:
    clock = Clock()
    path = tmp_path / "v4.sqlite"

    with RunningServer(setup_manager(path, clock)) as first:
        status, created = first.request("POST", "/api/v1/matches", {"handsPerTeam": 2})
        assert status == 201
        match_id = created["matchId"]
        status, joined = first.request(
            "POST",
            f"/api/v1/matches/{match_id}/join",
            {"inviteToken": created["inviteToken"]},
        )
        assert status == 200

    with RunningServer(setup_manager(path, clock)) as restarted:
        for seat, token in (
            (0, created["hostToken"]),
            (1, joined["guestToken"]),
        ):
            status, snapshot = restarted.request("GET", f"/api/v1/matches/{match_id}", token=token)
            assert status == 200
            assert snapshot["seat"] == seat
            assert snapshot["status"] == "active"


def test_http_expiry_returns_410(tmp_path: Path) -> None:
    clock = Clock()
    path = tmp_path / "v4.sqlite"

    with RunningServer(setup_manager(path, clock)) as server:
        _, created = server.request("POST", "/api/v1/matches", {"handsPerTeam": 2})
        clock.value = 1060

        status, payload = server.request(
            "GET",
            f"/api/v1/matches/{created['matchId']}",
            token=created["hostToken"],
        )
        assert status == 410
        assert payload["error"]["code"] == "MATCH_EXPIRED"


def test_http_retry_after_restart_does_not_roll_twice(
    tmp_path: Path,
) -> None:
    clock = Clock()
    path = tmp_path / "v4.sqlite"

    with RunningServer(setup_manager(path, clock)) as first:
        _, created = first.request("POST", "/api/v1/matches", {"handsPerTeam": 2})
        match_id = created["matchId"]
        first.request(
            "POST",
            f"/api/v1/matches/{match_id}/join",
            {"inviteToken": created["inviteToken"]},
        )
        body = {"requestId": "lost-response", "expectedRevision": 1}

        status, result = first.request(
            "POST",
            f"/api/v1/matches/{match_id}/roll",
            body,
            created["hostToken"],
        )
        assert status == 200

    with RunningServer(setup_manager(path, clock)) as restarted:
        status, replay = restarted.request(
            "POST",
            f"/api/v1/matches/{match_id}/roll",
            body,
            created["hostToken"],
        )
        assert status == 200
        assert replay["duplicate"] is True
        assert replay["revision"] == result["revision"]
        assert replay["face"] == result["face"]
