"""Adversarial tests of the isolated Dado Goal HTTP gateway."""

from __future__ import annotations

import http.client
from threading import Thread

import pytest

from boardlab.games.dado_goal_gateway import GatewayServer
from boardlab.games.dado_goal_http import create_local_server


@pytest.fixture
def gateway_pair():
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


def request(gateway, method, path, headers=None, body=None):
    connection = http.client.HTTPConnection("127.0.0.1", gateway.server_port, timeout=5)
    values = {"Host": f"127.0.0.1:{gateway.server_port}"}
    values.update(headers or {})
    try:
        connection.request(method, path, body=body, headers=values)
        response = connection.getresponse()
        return response.status, response.read()
    finally:
        connection.close()


@pytest.mark.parametrize(
    ("header", "value", "expected"),
    [
        ("Host", "attacker.invalid", 400),
        ("Origin", "https://attacker.invalid", 403),
        ("Origin", "null", 403),
    ],
)
def test_host_origin_rejection(gateway_pair, header, value, expected):
    status, _ = request(gateway_pair, "GET", "/multiplayer.html", {header: value})
    assert status == expected


@pytest.mark.parametrize(
    "header",
    ["X-Forwarded-Host", "X-Forwarded-Proto", "Forwarded"],
)
def test_forwarded_headers_do_not_override_origin(gateway_pair, header):
    status, body = request(
        gateway_pair,
        "GET",
        "/multiplayer.html",
        {header: "attacker.invalid"},
    )
    assert status == 200
    assert b"html" in body.lower()


@pytest.mark.parametrize("method", ["PUT", "DELETE", "PATCH"])
def test_unsupported_methods_rejected(gateway_pair, method):
    status, _ = request(gateway_pair, method, "/api/v1/matches")
    assert status in (405, 501)


def test_body_limit(gateway_pair):
    status, _ = request(
        gateway_pair,
        "POST",
        "/api/v1/matches",
        {"Content-Type": "application/json"},
        b"x" * 65537,
    )
    assert status == 413


def test_upstream_unavailable():
    gateway = GatewayServer(1)
    thread = Thread(target=gateway.serve_forever, daemon=True)
    thread.start()
    try:
        status, body = request(gateway, "GET", "/multiplayer.html")
        assert status == 502
        assert b"UPSTREAM_UNAVAILABLE" in body
    finally:
        gateway.shutdown()
        gateway.server_close()
        thread.join(timeout=5)
