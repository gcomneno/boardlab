"""Local authoritative Dado Goal match manager.

Laboratory prototype: memory-only, single process, no transport,
durable storage, expiry, rate limiting or distributed coordination.
"""

from __future__ import annotations

import secrets
from dataclasses import dataclass, field
from threading import RLock
from typing import Any

from boardlab.games.dado_goal import (
    PINK_FACES,
    RESULT_FACES,
    TEAM_FACES,
    Phase,
    PinkFace,
    ResultFace,
    State,
    TeamFace,
    apply_roll,
    initial_state,
)
from boardlab.games.dado_goal_wire import encode_state


class MatchError(ValueError):
    """Rejected multiplayer operation."""


FACES = {
    Phase.TEAM: TEAM_FACES,
    Phase.PINK: PINK_FACES,
    Phase.RESULT: RESULT_FACES,
}


@dataclass
class _Match:
    state: State
    invite: str
    seats: list[str | None]
    status: str = "waiting"
    revision: int = 0
    events: list[dict[str, Any]] = field(default_factory=list)
    receipts: dict[tuple[int, str], tuple[int, dict[str, Any]]] = field(default_factory=dict)
    lock: RLock = field(default_factory=RLock)


class MatchManager:
    """Authoritative single-process match registry."""

    def __init__(self, *, max_matches: int = 100):
        if type(max_matches) is not int or max_matches < 1:
            raise ValueError("Invalid match capacity")
        self._matches: dict[str, _Match] = {}
        self._capacity = max_matches
        self._lock = RLock()

    @staticmethod
    def _credential() -> str:
        return secrets.token_urlsafe(32)

    @staticmethod
    def _seat(match: _Match, token: str) -> int:
        if type(token) is not str:
            raise MatchError("Unauthorized participant")
        for seat, credential in enumerate(match.seats):
            if credential is not None and secrets.compare_digest(credential, token):
                return seat
        raise MatchError("Unauthorized participant")

    def _get(self, match_id: str) -> _Match:
        if type(match_id) is not str:
            raise MatchError("Unknown match")
        with self._lock:
            match = self._matches.get(match_id)
        if match is None:
            raise MatchError("Unknown match")
        return match

    @staticmethod
    def _snapshot(match: _Match, seat: int) -> dict[str, Any]:
        return {
            "revision": match.revision,
            "status": match.status,
            "seat": seat,
            "state": encode_state(match.state),
        }

    def create(self, hands_per_team: int) -> dict[str, Any]:
        if type(hands_per_team) is not int or not 1 <= hands_per_team <= 1000:
            raise MatchError("Invalid match length")

        with self._lock:
            if len(self._matches) >= self._capacity:
                raise MatchError("Match capacity reached")

            match_id = self._credential()
            while match_id in self._matches:
                match_id = self._credential()

            host_token = self._credential()
            invite = self._credential()

            match = _Match(
                state=initial_state(hands_per_team),
                invite=invite,
                seats=[host_token, None],
            )
            self._matches[match_id] = match

        return {
            "matchId": match_id,
            "hostToken": host_token,
            "inviteToken": invite,
            "snapshot": self._snapshot(match, 0),
        }

    def join(self, match_id: str, invite_token: str) -> dict[str, Any]:
        match = self._get(match_id)

        with match.lock:
            if type(invite_token) is not str or not secrets.compare_digest(
                invite_token, match.invite
            ):
                raise MatchError("Invalid invitation")

            if match.status != "waiting" or match.seats[1]:
                raise MatchError("Match is not joinable")

            guest_token = self._credential()
            match.seats[1] = guest_token
            match.status = "active"
            match.revision += 1

            return {
                "guestToken": guest_token,
                "snapshot": self._snapshot(match, 1),
            }

    def snapshot(self, match_id: str, seat_token: str) -> dict[str, Any]:
        match = self._get(match_id)
        with match.lock:
            seat = self._seat(match, seat_token)
            return self._snapshot(match, seat)

    def roll(
        self,
        match_id: str,
        seat_token: str,
        request_id: str,
        expected_revision: int,
    ) -> dict[str, Any]:
        match = self._get(match_id)

        with match.lock:
            seat = self._seat(match, seat_token)

            if type(request_id) is not str or not 1 <= len(request_id) <= 128:
                raise MatchError("Invalid request ID")

            if type(expected_revision) is not int:
                raise MatchError("Invalid revision")

            key = (seat, request_id)

            if key in match.receipts:
                original_revision, result = match.receipts[key]
                if expected_revision != original_revision:
                    raise MatchError("Request ID reused with other revision")
                return {
                    **result,
                    "duplicate": True,
                }

            if match.status != "active":
                raise MatchError("Match is not active")

            if expected_revision != match.revision:
                raise MatchError("Stale revision")

            if seat != match.state.active_player:
                raise MatchError("Not your turn")

            phase = match.state.phase
            if phase not in FACES:
                raise MatchError("No roll permitted")

            faces = FACES[phase]
            face = faces[secrets.randbelow(len(faces))]
            assert isinstance(face, (TeamFace, PinkFace, ResultFace))
            match.state = apply_roll(match.state, face)
            match.revision += 1

            if match.state.phase is Phase.FINISHED:
                match.status = "completed"

            result = {
                "revision": match.revision,
                "die": phase.value,
                "face": face.value,
                "status": match.status,
                "state": encode_state(match.state),
                "duplicate": False,
            }

            match.receipts[key] = (expected_revision, result.copy())
            match.events.append(
                {
                    "revision": match.revision,
                    "die": phase.value,
                    "face": face.value,
                }
            )

            return result
