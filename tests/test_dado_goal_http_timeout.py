"""Real socket tests for Dado Goal HTTP connection timeouts."""

from __future__ import annotations

import socket
import threading
from collections.abc import Iterator

import pytest

from boardlab.games.dado_goal_http import create_local_server


@pytest.fixture
def server() -> Iterator[object]:
    instance = create_local_server()
    instance.request_timeout_seconds = 0.25
    thread = threading.Thread(
        target=instance.serve_forever,
        daemon=True,
    )
    thread.start()
    try:
        yield instance
    finally:
        instance.shutdown()
        instance.server_close()
        thread.join(timeout=5)
        assert not thread.is_alive()


def connect(server: object) -> socket.socket:
    from boardlab.games.dado_goal_http import LocalHTTPServer

    assert isinstance(server, LocalHTTPServer)
    connection = socket.create_connection(
        ("127.0.0.1", server.server_port),
        timeout=3,
    )
    connection.settimeout(3)
    return connection


def test_incomplete_headers_cannot_hold_thread(
    server: object,
) -> None:
    with connect(server) as connection:
        connection.sendall(b"POST /api/v1/matches HTTP/1.1\r\nHost: 127.0.0.1")
        assert connection.recv(4096) == b""


def test_incomplete_body_cannot_hold_thread(
    server: object,
) -> None:
    from boardlab.games.dado_goal_http import LocalHTTPServer

    assert isinstance(server, LocalHTTPServer)

    with connect(server) as connection:
        request = (
            b"POST /api/v1/matches HTTP/1.1\r\n"
            + f"Host: 127.0.0.1:{server.server_port}\r\n".encode()
            + b"Content-Type: application/json\r\n"
            + b"Content-Length: 30\r\n"
            + b"\r\n"
            + b'{"handsPerTeam":'
        )
        connection.sendall(request)

        try:
            response = connection.recv(4096)
        except ConnectionResetError:
            response = b""

        assert response == b"" or b"HTTP/1.1" in response


def test_default_timeout_is_bounded() -> None:
    from boardlab.games.dado_goal_http import LocalHTTPServer

    assert 0 < LocalHTTPServer.request_timeout_seconds <= 10
