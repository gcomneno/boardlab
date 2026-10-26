"""Single-process, fixed-window HTTP throttling for Dado Goal.

Laboratory only. Not a distributed or proxy-aware security boundary.
"""

from __future__ import annotations

import math
import time
from collections.abc import Callable
from threading import Lock


class RateLimiter:
    """Bounded single-process rate limiter with injectable clock."""

    def __init__(
        self,
        *,
        limit: int,
        window_seconds: int,
        clock: Callable[[], float] | None = None,
    ) -> None:
        if type(limit) is not int or not 1 <= limit <= 10000:
            raise ValueError("Invalid rate limit")
        if type(window_seconds) is not int or not 1 <= window_seconds <= 86400:
            raise ValueError("Invalid rate window")
        self.limit = limit
        self.window_seconds = window_seconds
        self.clock = clock if clock is not None else time.monotonic
        self._lock = Lock()
        self._window_start = self._now()
        self._count = 0

    def _now(self) -> float:
        now = self.clock()
        if not isinstance(now, (int, float)) or isinstance(now, bool):
            raise ValueError("Invalid rate clock")
        if not math.isfinite(now) or now < 0:
            raise ValueError("Invalid rate clock")
        return float(now)

    def allow(self) -> bool:
        with self._lock:
            now = self._now()
            if now < self._window_start:
                raise ValueError("Rate clock moved backwards")
            if now - self._window_start >= self.window_seconds:
                self._window_start = now
                self._count = 0
            if self._count >= self.limit:
                return False
            self._count += 1
            return True
