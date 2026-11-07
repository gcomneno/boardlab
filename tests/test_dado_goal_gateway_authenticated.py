"""Complete authenticated gateway flow over real local HTTP."""

from __future__ import annotations

import http.client
import json
from pathlib import Path
from threading import Thread

from boardlab.games.dado_goal_gateway_authenticated import (
    AuthenticatedGatewayServer,
)
from boardlab.games.dado_goal_http import (
    create_lifecycle_local_server,
)
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy

KEY = "a" * 48


def test_login_gameplay_csrf_and_logout(tmp_path: Path) -> None:
    backend = create_lifecycle_local_server(
        tmp_path / "auth-v4.sqlite",
        LifecyclePolicy(3600, 7200, 86400),
    )
    gateway = AuthenticatedGatewayServer(
        backend.server_port,
        access_key=KEY,
    )
    threads = [
        Thread(target=backend.serve_forever, daemon=True),
        Thread(target=gateway.serve_forever, daemon=True),
    ]
    for thread in threads:
        thread.start()

    def request(
        method: str,
        path: str,
        body: dict | None = None,
        cookie: str | None = None,
        csrf: str | None = None,
        bearer: str | None = None,
    ):
        connection = http.client.HTTPConnection(
            "127.0.0.1", gateway.server_port, timeout=5
        )
        host = f"127.0.0.1:{gateway.server_port}"
        headers = {
            "Host": host,
            "Origin": f"http://{host}",
        }
        payload = None
        if body is not None:
            payload = json.dumps(body)
            headers["Content-Type"] = "application/json"
        if cookie is not None:
            headers["Cookie"] = cookie
        if csrf is not None:
            headers["X-Dado-CSRF"] = csrf
        if bearer is not None:
            headers["Authorization"] = f"Bearer {bearer}"

        try:
            connection.request(method, path, payload, headers)
            response = connection.getresponse()
            return (
                response.status,
                json.loads(response.read()),
                response.getheader("Set-Cookie"),
            )
        finally:
            connection.close()

    try:
        status, _, _ = request("GET", "/api/v1/matches/invalid")
        assert status == 403

        status, login, set_cookie = request(
            "POST", "/auth/login", {"accessKey": KEY}
        )
        assert status == 200
        assert set_cookie is not None
        assert "HttpOnly" in set_cookie
        assert "Secure" in set_cookie
        cookie = set_cookie.split(";", 1)[0]
        csrf = login["csrfToken"]

        status, _, _ = request(
            "POST", "/api/v1/matches", {"handsPerTeam": 2},
            cookie=cookie,
        )
        assert status == 403

        status, created, _ = request(
            "POST", "/api/v1/matches", {"handsPerTeam": 2},
            cookie=cookie, csrf=csrf,
        )
        assert status == 201
        match_id = created["matchId"]

        status, joined, _ = request(
            "POST", f"/api/v1/matches/{match_id}/join",
            {"inviteToken": created["inviteToken"]},
            cookie=cookie, csrf=csrf,
        )
        assert status == 200

        for token, seat in (
            (created["hostToken"], 0),
            (joined["guestToken"], 1),
        ):
            status, snapshot, _ = request(
                "GET", f"/api/v1/matches/{match_id}",
                cookie=cookie, bearer=token,
            )
            assert status == 200
            assert snapshot["seat"] == seat

        status, result, _ = request(
            "POST", f"/api/v1/matches/{match_id}/roll",
            {"requestId": "auth-e2e-1", "expectedRevision": 1},
            cookie=cookie, csrf=csrf,
            bearer=created["hostToken"],
        )
        assert status == 200
        assert result["revision"] == 2

        status, _, cleared = request(
            "POST", "/auth/logout",
            cookie=cookie, csrf=csrf,
        )
        assert status == 200
        assert cleared is not None
        assert "Max-Age=0" in cleared

        status, _, _ = request(
            "GET", f"/api/v1/matches/{match_id}",
            cookie=cookie, bearer=created["hostToken"],
        )
        assert status == 403

    finally:
        for server in (gateway, backend):
            server.shutdown()
            server.server_close()
        for thread in threads:
            thread.join(timeout=5)
