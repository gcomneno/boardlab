"""HTTPS-origin policy remains separate from internal HTTP forwarding."""

from __future__ import annotations

import http.client
from threading import Thread

import pytest

from boardlab.games.dado_goal_gateway import GatewayServer
from boardlab.games.dado_goal_http import create_local_server


def test_https_origin_rejects_invalid_configurations():
    for origin in (
        "http://localhost:1000",
        "https://example.com",
        "https://localhost",
        "https://localhost:1234",
        "https://localhost:1234/path",
    ):
        with pytest.raises(ValueError):
            GatewayServer(1, public_origin=origin)


def test_https_origin_maps_to_loopback_upstream():
    upstream = create_local_server()
    gateway = GatewayServer(upstream.server_port)
    gateway.public_origin = f"https://localhost:{gateway.server_port}"

    threads = [
        Thread(target=upstream.serve_forever, daemon=True),
        Thread(target=gateway.serve_forever, daemon=True),
    ]

    for thread in threads:
        thread.start()

    try:
        connection = http.client.HTTPConnection("127.0.0.1", gateway.server_port, timeout=5)
        try:
            connection.request(
                "GET",
                "/multiplayer.html",
                headers={
                    "Host": f"localhost:{gateway.server_port}",
                    "Origin": f"https://localhost:{gateway.server_port}",
                },
            )
            response = connection.getresponse()
            assert response.status == 200
            assert b"html" in response.read().lower()
        finally:
            connection.close()
    finally:
        for server in (gateway, upstream):
            server.shutdown()
            server.server_close()
        for thread in threads:
            thread.join(timeout=5)


def test_public_origin_configuration_and_exact_host():
    origin = "https://dado-goal-example.trycloudflare.com"
    gateway = GatewayServer(1, public_origin=origin)
    try:
        assert gateway.public_origin == origin
        assert gateway.server_address[0] == "127.0.0.1"
    finally:
        gateway.server_close()


@pytest.mark.parametrize(
    "origin",
    [
        "http://dado-goal-example.trycloudflare.com",
        "https://evil.example",
        "https://trycloudflare.com",
        "https://dado-goal-example.trycloudflare.com.evil.test",
        "https://dado-goal-example.trycloudflare.com/path",
        "https://dado-goal-example.trycloudflare.com:443",
        "https://user@dado-goal-example.trycloudflare.com",
    ],
)
def test_public_origin_rejects_invalid_values(origin):
    with pytest.raises(ValueError):
        GatewayServer(1, public_origin=origin)
