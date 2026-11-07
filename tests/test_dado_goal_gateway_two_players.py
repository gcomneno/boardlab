"""Two independent players through the isolated loopback gateway."""

from __future__ import annotations

import http.client
import json
from pathlib import Path
from threading import Thread
from typing import Any

from boardlab.games.dado_goal_gateway import GatewayServer
from boardlab.games.dado_goal_http import create_lifecycle_local_server
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy


def exchange(port: Any, method: Any, path: Any, payload: Any = None, token: Any = None) -> Any:
    connection = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    headers = {
        "Host": f"127.0.0.1:{port}",
        "Origin": f"http://127.0.0.1:{port}",
    }
    if token is not None:
        headers["Authorization"] = f"Bearer {token}"
    body = None
    if payload is not None:
        body = json.dumps(payload).encode()
        headers["Content-Type"] = "application/json"
    try:
        connection.request(method, path, body=body, headers=headers)
        response = connection.getresponse()
        data = json.loads(response.read())
        return response.status, data
    finally:
        connection.close()


def test_two_players_share_authoritative_v4_match(tmp_path: Path) -> None:
    authority = create_lifecycle_local_server(
        tmp_path / "gateway-e2e.sqlite",
        LifecyclePolicy(60, 120, 300),
    )
    gateway = GatewayServer(authority.server_port)
    threads = [
        Thread(target=authority.serve_forever, daemon=True),
        Thread(target=gateway.serve_forever, daemon=True),
    ]
    for thread in threads:
        thread.start()

    try:
        status, created = exchange(
            gateway.server_port,
            "POST",
            "/api/v1/matches",
            {"handsPerTeam": 2},
        )
        assert status == 201
        match_id = created["matchId"]
        host_token = created["hostToken"]

        status, joined = exchange(
            gateway.server_port,
            "POST",
            f"/api/v1/matches/{match_id}/join",
            {"inviteToken": created["inviteToken"]},
        )
        assert status == 200
        guest_token = joined["guestToken"]
        assert host_token != guest_token

        snapshot_path = f"/api/v1/matches/{match_id}"

        status, host_view = exchange(
            gateway.server_port,
            "GET",
            snapshot_path,
            token=host_token,
        )
        assert status == 200
        assert host_view["seat"] == 0
        assert host_view["status"] == "active"

        status, guest_view = exchange(
            gateway.server_port,
            "GET",
            snapshot_path,
            token=guest_token,
        )
        assert status == 200
        assert guest_view["seat"] == 1
        assert guest_view["status"] == "active"
        assert guest_view["revision"] == host_view["revision"] == 1

        status, unauthorized = exchange(
            gateway.server_port,
            "GET",
            snapshot_path,
            token=created["inviteToken"],
        )
        assert status == 401
        assert unauthorized["error"]["code"] == "UNAUTHORIZED"

        roll_path = f"/api/v1/matches/{match_id}/roll"
        roll_body = {
            "requestId": "gateway-e2e-roll-1",
            "expectedRevision": 1,
        }

        status, result = exchange(
            gateway.server_port,
            "POST",
            roll_path,
            roll_body,
            token=host_token,
        )
        assert status == 200
        assert result["revision"] == 2

        status, replay = exchange(
            gateway.server_port,
            "POST",
            roll_path,
            roll_body,
            token=host_token,
        )
        assert status == 200
        assert replay["duplicate"] is True
        assert replay["face"] == result["face"]

        for token, expected_seat in (
            (host_token, 0),
            (guest_token, 1),
        ):
            status, view = exchange(
                gateway.server_port,
                "GET",
                snapshot_path,
                token=token,
            )
            assert status == 200
            assert view["seat"] == expected_seat
            assert view["revision"] == 2

    finally:
        for server in (gateway, authority):
            server.shutdown()
            server.server_close()
        for thread in threads:
            thread.join(timeout=5)
