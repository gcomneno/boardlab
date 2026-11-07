"""Deterministic rate-limit and real loopback HTTP tests."""

from __future__ import annotations

import http.client
import json
import threading
from pathlib import Path

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


def test_fixed_window_and_reset() -> None:
    clock = Clock()
    limiter = RateLimiter(limit=2, window_seconds=10, clock=clock)
    assert limiter.allow()
    assert limiter.allow()
    assert not limiter.allow()
    clock.value = 9.99
    assert not limiter.allow()
    clock.value = 10
    assert limiter.allow()


@pytest.mark.parametrize("limit", [0, -1, True, 10001])
def test_invalid_rate_limit(limit: object) -> None:
    with pytest.raises(ValueError):
        RateLimiter(limit=limit, window_seconds=10)  # type: ignore[arg-type]


def test_http_429_for_create_join_and_roll(tmp_path: Path) -> None:
    manager = LifecycleAuthorityV4(
        tmp_path / "rate-v4.sqlite",
        LifecyclePolicy(60, 120, 300),
        clock=lambda: 1000,
    )
    manager.initialize()

    clock = Clock()
    limiter = RateLimiter(limit=4, window_seconds=60, clock=clock)
    server = create_rate_limited_local_server(manager, limiter)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()

    def request(
        method: str,
        path: str,
        body: dict[str, object],
        token: str | None = None,
    ) -> tuple[int, dict[str, object]]:
        connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=5)
        headers = {
            "Content-Type": "application/json",
            "Host": f"127.0.0.1:{server.server_port}",
        }
        if token is not None:
            headers["Authorization"] = f"Bearer {token}"
        try:
            connection.request(method, path, json.dumps(body), headers)
            response = connection.getresponse()
            return response.status, json.loads(response.read())
        finally:
            connection.close()

    try:
        status, created = request("POST", "/api/v1/matches", {"handsPerTeam": 2})
        assert status == 201
        match_id = created["matchId"]

        status, _ = request(
            "POST",
            f"/api/v1/matches/{match_id}/join",
            {"inviteToken": created["inviteToken"]},
        )
        assert status == 200

        status, _ = request(
            "POST",
            f"/api/v1/matches/{match_id}/roll",
            {"requestId": "r1", "expectedRevision": 1},
            created["hostToken"],
        )
        assert status == 200

        status, _ = request(
            "POST",
            f"/api/v1/matches/{match_id}/roll",
            {"requestId": "r1", "expectedRevision": 1},
            created["hostToken"],
        )
        assert status == 200

        status, payload = request("POST", "/api/v1/matches", {"handsPerTeam": 2})
        assert status == 429
        assert payload["error"]["code"] == "RATE_LIMITED"

        clock.value = 60
        status, _ = request("POST", "/api/v1/matches", {"handsPerTeam": 2})
        assert status == 201
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
