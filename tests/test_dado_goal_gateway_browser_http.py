"""Real HTTP qualification of isolated browser authentication."""

from __future__ import annotations

import http.client
import json
from threading import Thread

import pytest

from boardlab.games.dado_goal_gateway_browser_http import BrowserAuthServer

KEY = "a" * 48


@pytest.fixture
def server():
    instance = BrowserAuthServer(KEY)
    thread = Thread(target=instance.serve_forever, daemon=True)
    thread.start()
    try:
        yield instance
    finally:
        instance.shutdown()
        instance.server_close()
        thread.join(timeout=5)


def request(server, path, payload=None, *, cookie=None, csrf=None, origin=None):
    connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=5)
    headers = {
        "Host": f"127.0.0.1:{server.server_port}",
        "Content-Type": "application/json",
    }
    if cookie is not None:
        headers["Cookie"] = cookie
    if csrf is not None:
        headers["X-Dado-CSRF"] = csrf
    if origin is not None:
        headers["Origin"] = origin

    body = json.dumps(payload or {})
    try:
        connection.request("POST", path, body, headers)
        response = connection.getresponse()
        return (
            response.status,
            json.loads(response.read()),
            response.getheader("Set-Cookie"),
        )
    finally:
        connection.close()


def test_complete_login_csrf_logout_flow(server):
    status, _, _ = request(server, "/auth/login", {"accessKey": "wrong"})
    assert status == 403

    status, logged_in, set_cookie = request(server, "/auth/login", {"accessKey": KEY})
    assert status == 200
    assert set_cookie is not None
    assert "Secure" in set_cookie
    assert "HttpOnly" in set_cookie
    assert "SameSite=Strict" in set_cookie

    cookie = set_cookie.split(";", 1)[0]
    csrf = logged_in["csrfToken"]

    status, _, _ = request(server, "/auth/logout", cookie=cookie)
    assert status == 403

    status, result, cleared = request(server, "/auth/logout", cookie=cookie, csrf="wrong")
    assert status == 403
    assert cleared is None

    status, result, cleared = request(server, "/auth/logout", cookie=cookie, csrf=csrf)
    assert status == 200
    assert result["loggedOut"] is True
    assert cleared is not None
    assert "Max-Age=0" in cleared

    status, _, _ = request(server, "/auth/logout", cookie=cookie, csrf=csrf)
    assert status == 403


def test_cross_origin_login_rejected(server):
    status, _, cookie = request(
        server,
        "/auth/login",
        {"accessKey": KEY},
        origin="https://attacker.invalid",
    )
    assert status == 403
    assert cookie is None


def test_unknown_route_requires_authentication(server):
    status, _, _ = request(server, "/unknown")
    assert status == 403
