"""Independent, opt-in polling rate limit over real loopback HTTP."""

from __future__ import annotations

import http.client
import json
import threading
from collections.abc import Iterator
from pathlib import Path

import pytest

from boardlab.games.dado_goal_http import LocalHTTPServer, create_local_server
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import LifecycleAuthorityV4
from boardlab.games.dado_goal_rate_limit import RateLimiter


class Clock:
    def __init__(self) -> None:
        self.value = 0.0

    def __call__(self) -> float:
        return self.value


@pytest.fixture
def service(
    tmp_path: Path,
) -> Iterator[tuple[LocalHTTPServer, Clock]]:
    manager = LifecycleAuthorityV4(
        tmp_path / "poll.sqlite",
        LifecyclePolicy(60, 120, 300),
        clock=lambda: 1000,
    )
    manager.initialize()

    clock = Clock()
    server = create_local_server(manager)
    server.read_rate_limiter = RateLimiter(
        limit=2,
        window_seconds=30,
        clock=clock,
    )
    server.rate_limiter = RateLimiter(
        limit=3,
        window_seconds=30,
        clock=clock,
    )

    thread = threading.Thread(
        target=server.serve_forever,
        daemon=True,
    )
    thread.start()
    try:
        yield server, clock
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
        assert not thread.is_alive()


def send(
    server: LocalHTTPServer,
    method: str,
    path: str,
    body: dict[str, object] | None = None,
    token: str | None = None,
) -> tuple[int, dict[str, object]]:
    connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=5)
    headers = {"Host": f"127.0.0.1:{server.server_port}"}
    if body is not None:
        headers["Content-Type"] = "application/json"
    if token is not None:
        headers["Authorization"] = f"Bearer {token}"

    try:
        connection.request(
            method,
            path,
            None if body is None else json.dumps(body),
            headers,
        )
        response = connection.getresponse()
        return response.status, json.loads(response.read())
    finally:
        connection.close()


def test_poll_quota_independent_from_write_quota(
    service: tuple[LocalHTTPServer, Clock],
) -> None:
    server, _ = service

    status, created = send(
        server,
        "POST",
        "/api/v1/matches",
        {"handsPerTeam": 2},
    )
    assert status == 201

    path = f"/api/v1/matches/{created['matchId']}"
    token = created["hostToken"]

    for _ in range(2):
        status, snapshot = send(server, "GET", path, token=token)
        assert status == 200
        assert snapshot["status"] == "waiting"

    status, payload = send(server, "GET", path, token=token)
    assert status == 429
    assert payload["error"]["code"] == "RATE_LIMITED"

    # Excessive reads must not consume the independent write quota.
    status, _ = send(
        server,
        "POST",
        "/api/v1/matches",
        {"handsPerTeam": 2},
    )
    assert status == 201


def test_poll_quota_recovers_after_window(
    service: tuple[LocalHTTPServer, Clock],
) -> None:
    server, clock = service
    _, created = send(
        server,
        "POST",
        "/api/v1/matches",
        {"handsPerTeam": 2},
    )

    path = f"/api/v1/matches/{created['matchId']}"
    token = created["hostToken"]

    for _ in range(2):
        assert send(server, "GET", path, token=token)[0] == 200

    assert send(server, "GET", path, token=token)[0] == 429

    clock.value = 30
    assert send(server, "GET", path, token=token)[0] == 200


def test_static_assets_do_not_consume_poll_quota(
    service: tuple[LocalHTTPServer, Clock],
) -> None:
    server, _ = service

    # An invalid API route should not consume snapshot polling quota.
    for _ in range(4):
        status, _ = send(server, "GET", "/api/v1/unknown")
        assert status == 404

    _, created = send(
        server,
        "POST",
        "/api/v1/matches",
        {"handsPerTeam": 2},
    )

    path = f"/api/v1/matches/{created['matchId']}"
    assert (
        send(
            server,
            "GET",
            path,
            token=created["hostToken"],
        )[0]
        == 200
    )


def test_default_poll_limiter_is_opt_in() -> None:
    server = create_local_server()
    try:
        assert server.read_rate_limiter is None
    finally:
        server.server_close()
