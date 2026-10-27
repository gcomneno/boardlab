"""Isolated browser session contract tests."""

from __future__ import annotations

from typing import Any

import pytest

from boardlab.games.dado_goal_gateway_sessions import (
    GatewaySessions,
    session_cookie,
)


class Clock:
    value = 100.0

    def __call__(self) -> float:
        return self.value


def test_issue_validate_and_revoke() -> None:
    sessions = GatewaySessions()
    token = sessions.create()
    assert sessions.valid(token)
    assert not sessions.valid("incorrect")
    assert sessions.revoke(token)
    assert not sessions.valid(token)


def test_expiry() -> None:
    clock = Clock()
    sessions = GatewaySessions(ttl_seconds=10, clock=clock)
    token = sessions.create()
    clock.value = 109
    assert sessions.valid(token)
    clock.value = 110
    assert not sessions.valid(token)


def test_capacity_and_expired_cleanup() -> None:
    clock = Clock()
    sessions = GatewaySessions(ttl_seconds=10, max_sessions=1, clock=clock)
    first = sessions.create()
    with pytest.raises(RuntimeError):
        sessions.create()
    clock.value = 110
    second = sessions.create()
    assert second != first
    assert sessions.valid(second)


def test_cookie_security_attributes() -> None:
    token = GatewaySessions().create()
    cookie = session_cookie(token, 3600)
    assert cookie.startswith("__Host-dado-session=")
    assert "Path=/" in cookie
    assert "Secure" in cookie
    assert "HttpOnly" in cookie
    assert "SameSite=Strict" in cookie
    assert "Max-Age=3600" in cookie
    assert "Domain=" not in cookie


@pytest.mark.parametrize("ttl", [0, -1, True, 86401])
def test_invalid_ttl(ttl: Any) -> None:
    with pytest.raises(ValueError):
        GatewaySessions(ttl_seconds=ttl)


@pytest.mark.parametrize("capacity", [0, -1, True, 10001])
def test_invalid_capacity(capacity: Any) -> None:
    with pytest.raises(ValueError):
        GatewaySessions(max_sessions=capacity)
