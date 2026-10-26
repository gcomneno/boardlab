"""Real socket qualification of bounded Dado Goal HTTP connections."""

from __future__ import annotations

import socket
import threading
import time
from collections.abc import Iterator

import pytest

from boardlab.games.dado_goal_http import LocalHTTPServer, create_local_server


@pytest.fixture
def service() -> Iterator[LocalHTTPServer]:
    server = create_local_server()
    server.request_timeout_seconds = 2.0

    # A fresh semaphore with a capacity of two is installed before start.
    from threading import BoundedSemaphore

    server.max_connections = 2
    server._connection_slots = BoundedSemaphore(2)

    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield server
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
        assert not thread.is_alive()


def open_partial(server: LocalHTTPServer) -> socket.socket:
    sock = socket.create_connection(("127.0.0.1", server.server_port), timeout=3)
    sock.settimeout(3)
    sock.sendall(b"GET /multiplayer.html HTTP/1.1\r\nHost:")
    return sock


def test_excess_connection_does_not_consume_worker(
    service: LocalHTTPServer,
) -> None:
    first = open_partial(service)
    second = open_partial(service)

    try:
        # Wait until the two slots are actively held.
        deadline = time.monotonic() + 3
        while service._connection_slots._value != 0:
            assert time.monotonic() < deadline
            time.sleep(0.01)

        with socket.create_connection(("127.0.0.1", service.server_port), timeout=3) as excess:
            excess.settimeout(3)
            excess.sendall(b"GET /multiplayer.html HTTP/1.1\r\n\r\n")
            assert excess.recv(4096) == b""

        assert service._connection_slots._value == 0
    finally:
        first.close()
        second.close()


def test_capacity_is_recovered_after_disconnect(
    service: LocalHTTPServer,
) -> None:
    with open_partial(service):
        pass

    deadline = time.monotonic() + 3
    while service._connection_slots._value != 2:
        assert time.monotonic() < deadline
        time.sleep(0.01)

    with socket.create_connection(("127.0.0.1", service.server_port), timeout=3) as sock:
        sock.settimeout(3)
        sock.sendall(
            b"GET /multiplayer.html HTTP/1.1\r\n"
            + f"Host: 127.0.0.1:{service.server_port}\r\n".encode()
            + b"\r\n"
        )
        assert b"HTTP/1.1 200" in sock.recv(4096)


def test_default_connection_capacity() -> None:
    server = create_local_server()
    try:
        assert server.max_connections == 32
    finally:
        server.server_close()
