"""Isolated browser authentication policy for Dado Goal.

No HTTP routing or public deployment is authorized here.
"""

from __future__ import annotations

import hmac
import secrets
from dataclasses import dataclass
from http.cookies import CookieError, SimpleCookie

from boardlab.games.dado_goal_gateway_sessions import (
    GatewaySessions,
    session_cookie,
)

COOKIE_NAME = "__Host-dado-session"


@dataclass(frozen=True)
class BrowserSession:
    token: str
    csrf_token: str


class BrowserAuthentication:
    def __init__(
        self,
        *,
        access_key: str,
        sessions: GatewaySessions,
    ) -> None:
        if (
            not isinstance(access_key, str)
            or not 32 <= len(access_key) <= 256
            or not access_key.isascii()
            or not access_key.isprintable()
        ):
            raise ValueError("Invalid browser access key")

        self._access_key = access_key
        self._sessions = sessions
        self._csrf: dict[str, str] = {}

    def login(self, supplied_key: str) -> BrowserSession | None:
        if not isinstance(supplied_key, str):
            return None
        if not hmac.compare_digest(supplied_key, self._access_key):
            return None

        token = self._sessions.create()
        csrf = secrets.token_urlsafe(32)
        self._csrf[token] = csrf
        return BrowserSession(token, csrf)

    def authenticate(
        self,
        cookie_header: str | None,
        *,
        csrf_header: str | None = None,
        mutation: bool = False,
    ) -> bool:
        token = self._extract_cookie(cookie_header)
        if token is None or not self._sessions.valid(token):
            return False

        if mutation:
            expected = self._csrf.get(token)
            if (
                expected is None
                or not isinstance(csrf_header, str)
                or not hmac.compare_digest(csrf_header, expected)
            ):
                return False

        return True

    def logout(self, cookie_header: str | None) -> bool:
        token = self._extract_cookie(cookie_header)
        if token is None:
            return False

        self._csrf.pop(token, None)
        return self._sessions.revoke(token)

    def cookie(self, session: BrowserSession) -> str:
        return session_cookie(session.token, self._sessions.ttl_seconds)

    @staticmethod
    def _extract_cookie(header: str | None) -> str | None:
        if not isinstance(header, str) or len(header) > 4096:
            return None
        try:
            cookies = SimpleCookie()
            cookies.load(header)
        except CookieError:
            return None

        item = cookies.get(COOKIE_NAME)
        return item.value if item is not None else None
