"""Adversarial tests of optional loopback HTTP rate limiting."""

from __future__ import annotations

import http.client
import json
import threading
from pathlib import Path
from typing import Any, cast

import pytest

from boardlab.games.dado_goal_http import create_rate_limited_local_server
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import LifecycleAuthorityV4
from boardlab.games.dado_goal_rate_limit import RateLimiter


class Clock:
    def __init__(self) -> None:
        self.value = 0.0

    def __call__(self) -> float:
        return self.value


@pytest.fixture
def service(tmp_path: Path) -> Any:
    manager = LifecycleAuthorityV4(
        tmp_path / "security.sqlite",
        LifecyclePolicy(60, 120, 300),
        clock=lambda: 1000,
    )
    manager.initialize()
    clock = Clock()
    limiter = RateLimiter(limit=3, window_seconds=30, clock=clock)
    server = create_rate_limited_local_server(manager, limiter)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield server, clock
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


def send(
    server: Any,
    method: str,
    path: str,
    body: dict[str, object] | None = None,
    token: str | None = None,
    content_type: str = "application/json",
) -> tuple[int, dict[str, object]]:
    conn = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=5)
    headers = {
        "Host": f"127.0.0.1:{server.server_port}",
        "Content-Type": content_type,
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    try:
        conn.request(
            method,
            path,
            None if body is None else json.dumps(body),
            headers,
        )
        res = conn.getresponse()
        return res.status, json.loads(res.read())
    finally:
        conn.close()


def test_invalid_json_media_consumes_quota(service: Any) -> None:
    server, _ = service
    for _ in range(3):
        status, _ = send(
            server,
            "POST",
            "/api/v1/matches",
            {"handsPerTeam": 2},
            content_type="text/plain",
        )
        assert status == 415

    status, payload = send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    assert status == 429
    assert cast(dict[str, object], payload["error"])["code"] == "RATE_LIMITED"


def test_failed_invitation_consumes_quota(service: Any) -> None:
    server, _ = service
    status, created = send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    assert status == 201

    for _ in range(2):
        status, _ = send(
            server,
            "POST",
            f"/api/v1/matches/{created['matchId']}/join",
            {"inviteToken": "wrong"},
        )
        assert status == 401

    status, _ = send(
        server,
        "POST",
        f"/api/v1/matches/{created['matchId']}/join",
        {"inviteToken": created["inviteToken"]},
    )
    assert status == 429


def test_window_restores_service(service: Any) -> None:
    server, clock = service
    for _ in range(3):
        send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})

    status, _ = send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    assert status == 429

    clock.value = 30
    status, _ = send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    assert status == 201


def test_get_snapshot_does_not_consume_write_quota(service: Any) -> None:
    server, _ = service
    _, created = send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})

    for _ in range(10):
        status, _ = send(
            server,
            "GET",
            f"/api/v1/matches/{created['matchId']}",
            token=cast(str, created["hostToken"]),
        )
        assert status == 200

    status, _ = send(server, "POST", "/api/v1/matches", {"handsPerTeam": 2})
    assert status == 201
