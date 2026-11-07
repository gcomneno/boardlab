"""Browser navigation compatibility with authenticated Dado Goal gateway."""

from __future__ import annotations

import http.client
import json
from threading import Thread

from boardlab.games.dado_goal_gateway_authenticated import (
    AuthenticatedGatewayServer,
)
from boardlab.games.dado_goal_http import create_local_server

KEY = "a" * 48


def test_browser_navigation_and_mutation_origin_policy() -> None:
    backend = create_local_server()
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
        *,
        origin: str | None = None,
        cookie: str | None = None,
        csrf: str | None = None,
        body: dict | None = None,
    ) -> tuple[int, bytes, str | None]:
        connection = http.client.HTTPConnection(
            "127.0.0.1", gateway.server_port, timeout=5
        )
        host = f"127.0.0.1:{gateway.server_port}"
        headers = {"Host": host}
        if origin is not None:
            headers["Origin"] = origin
        if cookie is not None:
            headers["Cookie"] = cookie
        if csrf is not None:
            headers["X-Dado-CSRF"] = csrf

        payload = None
        if body is not None:
            payload = json.dumps(body)
            headers["Content-Type"] = "application/json"

        try:
            connection.request(method, path, payload, headers)
            response = connection.getresponse()
            return (
                response.status,
                response.read(),
                response.getheader("Set-Cookie"),
            )
        finally:
            connection.close()

    try:
        host = f"127.0.0.1:{gateway.server_port}"
        origin = f"http://{host}"

        # The game shell is public; gameplay APIs require authentication.
        status, page, _ = request("GET", "/multiplayer.html")
        assert status == 200
        assert b"html" in page.lower()

        status, body, set_cookie = request(
            "POST",
            "/auth/login",
            origin=origin,
            body={"accessKey": KEY},
        )
        assert status == 200
        assert set_cookie is not None
        csrf = json.loads(body)["csrfToken"]
        cookie = set_cookie.split(";", 1)[0]

        status, page, _ = request(
            "GET", "/multiplayer.html", cookie=cookie
        )
        assert status == 200
        assert b"html" in page.lower()

        status, _, _ = request(
            "GET",
            "/multiplayer.html",
            cookie=cookie,
            origin="https://attacker.invalid",
        )
        assert status == 403

        status, _, _ = request(
            "POST",
            "/api/v1/matches",
            cookie=cookie,
            csrf=csrf,
            body={"handsPerTeam": 2},
        )
        assert status == 403

        status, body, _ = request(
            "POST",
            "/api/v1/matches",
            origin=origin,
            cookie=cookie,
            csrf=csrf,
            body={"handsPerTeam": 2},
        )
        assert status == 201
        assert "matchId" in json.loads(body)

    finally:
        for server in (gateway, backend):
            server.shutdown()
            server.server_close()
        for thread in threads:
            thread.join(timeout=5)
