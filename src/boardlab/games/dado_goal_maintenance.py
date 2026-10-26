"""Explicit, bounded SQLite v4 maintenance coordinator.

No threads, implicit scheduling, database migrations, or startup cleanup.
"""

from __future__ import annotations

import math
import time
from collections.abc import Callable
from threading import Lock

from boardlab.games.dado_goal_lifecycle_authority import LifecycleAuthorityV4


class LifecycleMaintenance:
    """Run bounded cleanup on explicit calls, at configured intervals."""

    def __init__(
        self,
        authority: LifecycleAuthorityV4,
        *,
        interval_seconds: int,
        batch_limit: int,
        clock: Callable[[], float] | None = None,
    ) -> None:
        if not isinstance(authority, LifecycleAuthorityV4):
            raise TypeError("LifecycleAuthorityV4 required")
        if type(interval_seconds) is not int or not 1 <= interval_seconds <= 86400:
            raise ValueError("Invalid maintenance interval")
        if type(batch_limit) is not int or not 1 <= batch_limit <= 1000:
            raise ValueError("Invalid maintenance batch")

        self.authority = authority
        self.interval_seconds = interval_seconds
        self.batch_limit = batch_limit
        self.clock = clock if clock is not None else time.monotonic
        self._lock = Lock()
        self._last_run: float | None = None

    def _now(self) -> float:
        now = self.clock()
        if isinstance(now, bool) or not isinstance(now, (int, float)):
            raise ValueError("Invalid maintenance clock")
        if not math.isfinite(now) or now < 0:
            raise ValueError("Invalid maintenance clock")
        return float(now)

    def run_if_due(self) -> int | None:
        """Return deleted matches, or None when the interval has not elapsed."""
        with self._lock:
            now = self._now()
            if self._last_run is not None:
                if now < self._last_run:
                    raise ValueError("Maintenance clock moved backwards")
                if now - self._last_run < self.interval_seconds:
                    return None

            # A failed cleanup remains retryable on the next explicit call.
            deleted = self.authority.cleanup_expired(limit=self.batch_limit)
            self._last_run = now
            return deleted
