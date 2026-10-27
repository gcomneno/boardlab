"""Browser session login, CSRF and logout qualification."""

from __future__ import annotations

from typing import Any

from boardlab.games.dado_goal_gateway_browser import BrowserAuthentication
from boardlab.games.dado_goal_gateway_sessions import GatewaySessions

KEY = "a" * 48


def make_auth() -> Any:
    return BrowserAuthentication(
        access_key=KEY,
        sessions=GatewaySessions(),
    )


def test_login_requires_correct_key() -> None:
    auth = make_auth()
    assert auth.login("wrong") is None
    assert auth.login(KEY) is not None


def test_session_auth_and_csrf() -> None:
    auth = make_auth()
    session = auth.login(KEY)
    assert session is not None

    cookie = f"__Host-dado-session={session.token}"

    assert auth.authenticate(cookie)
    assert not auth.authenticate(cookie, mutation=True)
    assert not auth.authenticate(cookie, csrf_header="wrong", mutation=True)
    assert auth.authenticate(
        cookie,
        csrf_header=session.csrf_token,
        mutation=True,
    )


def test_logout_revokes_session() -> None:
    auth = make_auth()
    session = auth.login(KEY)
    assert session is not None

    cookie = f"__Host-dado-session={session.token}"

    assert auth.authenticate(cookie)
    assert auth.logout(cookie)
    assert not auth.authenticate(cookie)


def test_cookie_attributes() -> None:
    auth = make_auth()
    session = auth.login(KEY)
    assert session is not None

    cookie = auth.cookie(session)

    assert "Secure" in cookie
    assert "HttpOnly" in cookie
    assert "SameSite=Strict" in cookie
    assert "Path=/" in cookie
    assert "Domain=" not in cookie


def test_malformed_cookie_rejected() -> None:
    auth = make_auth()
    assert not auth.authenticate(None)
    assert not auth.authenticate("x" * 5000)
    assert not auth.authenticate("invalid")
