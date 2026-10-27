"""Local HTTP gateway integration with the existing Dado Goal server."""

from __future__ import annotations

import http.client
from threading import Thread

import pytest

from boardlab.games.dado_goal_gateway import GatewayServer
from boardlab.games.dado_goal_http import create_local_server


@pytest.fixture
def pair():
    upstream = create_local_server()
    gateway = GatewayServer(upstream.server_port)
    threads = [
        Thread(target=upstream.serve_forever, daemon=True),
        Thread(target=gateway.serve_forever, daemon=True),
    ]
    for thread in threads:
        thread.start()
    try:
        yield gateway
    finally:
        for server in (gateway, upstream):
            server.shutdown()
            server.server_close()
        for thread in threads:
            thread.join(timeout=5)


def request(gateway, method, path, *, host=None, origin=None, body=None):
    connection = http.client.HTTPConnection("127.0.0.1", gateway.server_port, timeout=5)
    headers = {
        "Host": host or f"127.0.0.1:{gateway.server_port}",
    }
    if origin is not None:
        headers["Origin"] = origin
    if body is not None:
        headers["Content-Type"] = "application/json"
    connection.request(method, path, body=body, headers=headers)
    response = connection.getresponse()
    result = (response.status, response.read())
    connection.close()
    return result


def test_create_match_through_gateway(pair):
    status, body = request(
        pair,
        "POST",
        "/api/v1/matches",
        origin=f"http://127.0.0.1:{pair.server_port}",
        body=b'{"handsPerTeam":2}',
    )
    assert status == 201
    assert b"matchId" in body


def test_wrong_host_rejected(pair):
    status, _ = request(pair, "GET", "/multiplayer.html", host="evil.example")
    assert status == 400


def test_wrong_origin_rejected(pair):
    status, _ = request(
        pair,
        "POST",
        "/api/v1/matches",
        origin="https://evil.example",
        body=b'{"handsPerTeam":2}',
    )
    assert status == 403


def test_assets_reachable(pair):
    status, body = request(pair, "GET", "/multiplayer.html")
    assert status == 200
    assert b"html" in body.lower()


def test_query_rejected(pair):
    status, _ = request(pair, "GET", "/multiplayer.html?token=secret")
    assert status == 400
