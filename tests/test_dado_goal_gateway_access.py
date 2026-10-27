"""Optional access-key isolation for the loopback gateway."""

from __future__ import annotations

import http.client
from threading import Thread
from typing import Any

import pytest

from boardlab.games.dado_goal_gateway import GatewayServer
from boardlab.games.dado_goal_http import create_local_server

KEY = "a" * 48


@pytest.mark.parametrize("invalid", ["", "short", "x" * 257, "è" * 48])
def test_invalid_access_key(invalid: Any) -> None:
    with pytest.raises(ValueError):
        GatewayServer(1, access_key=invalid)


def test_access_key_required_when_configured() -> None:
    upstream = create_local_server()
    gateway = GatewayServer(upstream.server_port, access_key=KEY)

    threads = [
        Thread(target=upstream.serve_forever, daemon=True),
        Thread(target=gateway.serve_forever, daemon=True),
    ]
    for thread in threads:
        thread.start()

    try:
        for supplied, expected in (
            (None, 403),
            ("b" * 48, 403),
            (KEY, 200),
        ):
            connection = http.client.HTTPConnection("127.0.0.1", gateway.server_port, timeout=5)
            headers = {"Host": f"127.0.0.1:{gateway.server_port}"}
            if supplied is not None:
                headers["X-Dado-Access-Key"] = supplied

            try:
                connection.request("GET", "/multiplayer.html", headers=headers)
                response = connection.getresponse()
                assert response.status == expected
                response.read()
            finally:
                connection.close()
    finally:
        for server in (gateway, upstream):
            server.shutdown()
            server.server_close()
        for thread in threads:
            thread.join(timeout=5)
