"""Bounded, single-process gateway browser sessions.

Laboratory only. Session persistence and browser HTTP integration
are separate qualification steps.
"""

from __future__ import annotations

import hmac
import math
import secrets
import time
from collections.abc import Callable
from threading import Lock


class GatewaySessions:
    def __init__(
        self,
        *,
        ttl_seconds: int = 3600,
        max_sessions: int = 32,
        clock: Callable[[], float] = time.monotonic,
    ) -> None:
        if type(ttl_seconds) is not int or not 1 <= ttl_seconds <= 86400:
            raise ValueError("Invalid session TTL")
        if type(max_sessions) is not int or not 1 <= max_sessions <= 10000:
            raise ValueError("Invalid session capacity")

        self.ttl_seconds = ttl_seconds
        self.max_sessions = max_sessions
        self._clock = clock
        self._sessions: dict[str, float] = {}
        self._lock = Lock()

    def _now(self) -> float:
        value = self._clock()
        if (
            isinstance(value, bool)
            or not isinstance(value, (int, float))
            or not math.isfinite(value)
            or value < 0
        ):
            raise ValueError("Invalid session clock")
        return float(value)

    def _cleanup(self, now: float) -> None:
        expired = [
            token
            for token, deadline in self._sessions.items()
            if deadline <= now
        ]
        for token in expired:
            del self._sessions[token]

    def create(self) -> str:
        with self._lock:
            now = self._now()
            self._cleanup(now)
            if len(self._sessions) >= self.max_sessions:
                raise RuntimeError("Session capacity reached")

            token = secrets.token_urlsafe(32)
            while token in self._sessions:
                token = secrets.token_urlsafe(32)

            self._sessions[token] = now + self.ttl_seconds
            return token

    def valid(self, token: str) -> bool:
        if not isinstance(token, str) or not token:
            return False

        with self._lock:
            now = self._now()
            self._cleanup(now)
            return any(
                hmac.compare_digest(token, active)
                for active in self._sessions
            )

    def revoke(self, token: str) -> bool:
        if not isinstance(token, str):
            return False

        with self._lock:
            for active in self._sessions:
                if hmac.compare_digest(token, active):
                    del self._sessions[active]
                    return True
            return False


def session_cookie(token: str, max_age: int) -> str:
    if not isinstance(token, str) or not token:
        raise ValueError("Invalid session token")
    if type(max_age) is not int or not 1 <= max_age <= 86400:
        raise ValueError("Invalid cookie lifetime")

    return (
        f"__Host-dado-session={token}; "
        "Path=/; Secure; HttpOnly; SameSite=Strict; "
        f"Max-Age={max_age}"
    )
