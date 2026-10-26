"""Isolated Dado Goal SQLite v4 authority foundation.

Implements create, join and snapshot. Roll and cleanup are
intentionally out of scope. No public HTTP exposure.
"""

from __future__ import annotations

import hashlib
import hmac
import json
import secrets
import sqlite3
import time
from collections.abc import Callable, Iterator
from contextlib import contextmanager
from pathlib import Path
from typing import Any

from boardlab.games.dado_goal import (
    PINK_FACES,
    RESULT_FACES,
    TEAM_FACES,
    Phase,
    PinkFace,
    ResultFace,
    TeamFace,
    apply_roll,
    initial_state,
)
from boardlab.games.dado_goal_lifecycle import (
    LifecyclePolicy,
    LifecycleRecord,
    LifecycleStatus,
    create_record,
    is_expired,
    transition,
)
from boardlab.games.dado_goal_lifecycle_store import LifecycleDatabaseV4
from boardlab.games.dado_goal_wire import decode_state, encode_state


class LifecycleAuthorityError(ValueError):
    """Rejected v4 authoritative operation."""


def _digest(token: str) -> bytes:
    if type(token) is not str or not 1 <= len(token) <= 256:
        raise LifecycleAuthorityError("Invalid credential")
    return hashlib.sha256(token.encode("utf-8")).digest()


def _token() -> str:
    return secrets.token_urlsafe(32)


class LifecycleAuthorityV4(LifecycleDatabaseV4):
    """Transactional v4 authority with a configurable server clock."""

    def __init__(
        self,
        path: str | Path,
        policy: LifecyclePolicy,
        *,
        clock: Callable[[], int] | None = None,
        max_matches: int | None = None,
    ) -> None:
        super().__init__(path)
        if not isinstance(policy, LifecyclePolicy):
            raise TypeError("LifecyclePolicy required")
        if max_matches is not None and (
            type(max_matches) is not int or not 1 <= max_matches <= 1_000_000
        ):
            raise ValueError("Invalid match capacity")
        self.max_matches = max_matches
        self.policy = policy
        self.clock = clock if clock is not None else lambda: int(time.time())

    def _now(self) -> int:
        value = self.clock()
        if type(value) is not int or value < 0:
            raise LifecycleAuthorityError("Invalid server clock")
        return value

    @contextmanager
    def _write(self) -> Iterator[sqlite3.Connection]:
        db = self._connect()
        try:
            db.execute("BEGIN IMMEDIATE")
            try:
                yield db
                db.commit()
            except BaseException:
                db.rollback()
                raise
        finally:
            db.close()

    @staticmethod
    def _seat(
        db: sqlite3.Connection,
        match_id: str,
        token: str,
    ) -> int:
        candidate = _digest(token)
        rows = db.execute(
            "SELECT seat,token_digest FROM lifecycle_seats WHERE match_id=? ORDER BY seat",
            (match_id,),
        ).fetchall()
        for seat, digest in rows:
            if hmac.compare_digest(bytes(digest), candidate):
                return int(seat)
        raise LifecycleAuthorityError("Unauthorized participant")

    @staticmethod
    def _match(
        db: sqlite3.Connection,
        match_id: str,
    ) -> tuple[Any, ...]:
        row = db.execute(
            "SELECT revision,status,state_json,created_at,"
            "updated_at,expires_at,completed_at "
            "FROM lifecycle_matches WHERE match_id=?",
            (match_id,),
        ).fetchone()
        if row is None:
            raise LifecycleAuthorityError("Unknown match")
        return tuple(row)

    @staticmethod
    def _snapshot(
        row: tuple[Any, ...],
        seat: int,
    ) -> dict[str, Any]:
        return {
            "revision": row[0],
            "status": row[1],
            "seat": seat,
            "state": encode_state(decode_state(__import__("json").loads(row[2]))),
        }

    def create(self, hands_per_team: int) -> dict[str, Any]:
        if type(hands_per_team) is not int or not 1 <= hands_per_team <= 1000:
            raise LifecycleAuthorityError("Invalid match length")

        now = self._now()
        lifecycle = create_record(self.policy, now)
        match_id = _token()
        host = _token()
        invitation = _token()

        state = encode_state(initial_state(hands_per_team))
        import json

        payload = json.dumps(state, sort_keys=True, separators=(",", ":"))

        with self._write() as db:
            if self.max_matches is not None:
                count = db.execute("SELECT COUNT(*) FROM lifecycle_matches").fetchone()[0]
                if count >= self.max_matches:
                    raise LifecycleAuthorityError("Match capacity reached")
            db.execute(
                "INSERT INTO lifecycle_matches "
                "(match_id,revision,status,state_json,"
                "invite_digest,invite_used,created_at,"
                "updated_at,expires_at,completed_at) "
                "VALUES (?,0,'waiting',?, ?,0,?,?,?,NULL)",
                (
                    match_id,
                    payload,
                    _digest(invitation),
                    lifecycle.created_at,
                    lifecycle.updated_at,
                    lifecycle.expires_at,
                ),
            )
            db.execute(
                "INSERT INTO lifecycle_seats VALUES (?,0,?)",
                (match_id, _digest(host)),
            )
            snapshot = self._snapshot(self._match(db, match_id), 0)

        return {
            "matchId": match_id,
            "hostToken": host,
            "inviteToken": invitation,
            "snapshot": snapshot,
        }

    def join(self, match_id: str, invite_token: str) -> dict[str, Any]:
        candidate = _digest(invite_token)

        with self._write() as db:
            record = db.execute(
                "SELECT status,invite_digest,invite_used,"
                "created_at,updated_at,expires_at "
                "FROM lifecycle_matches WHERE match_id=?",
                (match_id,),
            ).fetchone()
            if record is None or record[0] != "waiting" or record[2]:
                raise LifecycleAuthorityError("Match is not joinable")

            from boardlab.games.dado_goal_lifecycle import LifecycleRecord

            lifecycle = LifecycleRecord(
                status=LifecycleStatus.WAITING,
                created_at=record[3],
                updated_at=record[4],
                expires_at=record[5],
            )
            now = self._now()
            if is_expired(lifecycle, now):
                raise LifecycleAuthorityError("Match expired")

            if record[1] is None or not hmac.compare_digest(bytes(record[1]), candidate):
                raise LifecycleAuthorityError("Invalid invitation")

            updated = transition(lifecycle, self.policy, LifecycleStatus.ACTIVE, now)
            guest = _token()
            db.execute(
                "INSERT INTO lifecycle_seats VALUES (?,1,?)",
                (match_id, _digest(guest)),
            )
            db.execute(
                "UPDATE lifecycle_matches SET "
                "status='active',revision=revision+1,"
                "invite_digest=NULL,invite_used=1,"
                "updated_at=?,expires_at=? WHERE match_id=?",
                (updated.updated_at, updated.expires_at, match_id),
            )
            revision = self._match(db, match_id)[0]
            db.execute(
                "INSERT INTO lifecycle_events VALUES (?,?,?)",
                (match_id, revision, '{"type":"joined"}'),
            )
            snapshot = self._snapshot(self._match(db, match_id), 1)

        return {"guestToken": guest, "snapshot": snapshot}

    def snapshot(
        self,
        match_id: str,
        seat_token: str,
    ) -> dict[str, Any]:
        db = self._connect()
        try:
            seat = self._seat(db, match_id, seat_token)
            row = self._match(db, match_id)
            if self._now() >= row[5]:
                raise LifecycleAuthorityError("Match expired")
            return self._snapshot(row, seat)
        finally:
            db.close()

    def roll(
        self,
        match_id: str,
        seat_token: str,
        request_id: str,
        expected_revision: int,
    ) -> dict[str, Any]:
        """Commit a single authoritative die roll and durable receipt."""
        if type(request_id) is not str or not 1 <= len(request_id) <= 128:
            raise LifecycleAuthorityError("Invalid request ID")
        if type(expected_revision) is not int or expected_revision < 0:
            raise LifecycleAuthorityError("Invalid revision")

        faces_by_phase = {
            Phase.TEAM: TEAM_FACES,
            Phase.PINK: PINK_FACES,
            Phase.RESULT: RESULT_FACES,
        }

        with self._write() as db:
            seat = self._seat(db, match_id, seat_token)
            row = self._match(db, match_id)
            now = self._now()

            if now >= row[5]:
                raise LifecycleAuthorityError("Match expired")

            receipt = db.execute(
                "SELECT expected_revision,response_json "
                "FROM lifecycle_receipts "
                "WHERE match_id=? AND seat=? AND request_id=?",
                (match_id, seat, request_id),
            ).fetchone()

            if receipt is not None:
                if receipt[0] != expected_revision:
                    raise LifecycleAuthorityError("Request ID reused with other revision")
                return {**json.loads(receipt[1]), "duplicate": True}

            revision, status = row[0], row[1]

            if status != "active":
                raise LifecycleAuthorityError("Match is not active")
            if revision != expected_revision:
                raise LifecycleAuthorityError("Stale revision")

            state = decode_state(json.loads(row[2]))
            if state.active_player != seat:
                raise LifecycleAuthorityError("Not your turn")

            faces = faces_by_phase.get(state.phase)
            if faces is None:
                raise LifecycleAuthorityError("No roll permitted")

            phase = state.phase
            face = faces[secrets.randbelow(len(faces))]
            assert isinstance(face, (TeamFace, PinkFace, ResultFace))
            next_state = apply_roll(state, face)

            lifecycle = LifecycleRecord(
                status=LifecycleStatus.ACTIVE,
                created_at=row[3],
                updated_at=row[4],
                expires_at=row[5],
            )
            new_status = (
                LifecycleStatus.COMPLETED
                if next_state.phase is Phase.FINISHED
                else LifecycleStatus.ACTIVE
            )
            updated = transition(lifecycle, self.policy, new_status, now)
            next_revision = revision + 1

            result = {
                "revision": next_revision,
                "die": phase.value,
                "face": face.value,
                "status": new_status.value,
                "state": encode_state(next_state),
                "duplicate": False,
            }

            db.execute(
                "UPDATE lifecycle_matches SET "
                "revision=?,status=?,state_json=?,"
                "updated_at=?,expires_at=?,completed_at=? "
                "WHERE match_id=? AND revision=?",
                (
                    next_revision,
                    new_status.value,
                    json.dumps(
                        encode_state(next_state),
                        sort_keys=True,
                        separators=(",", ":"),
                    ),
                    updated.updated_at,
                    updated.expires_at,
                    updated.completed_at,
                    match_id,
                    revision,
                ),
            )

            db.execute(
                "INSERT INTO lifecycle_events VALUES (?,?,?)",
                (
                    match_id,
                    next_revision,
                    json.dumps(
                        {
                            "type": "roll",
                            "seat": seat,
                            "die": phase.value,
                            "face": face.value,
                        },
                        sort_keys=True,
                    ),
                ),
            )

            db.execute(
                "INSERT INTO lifecycle_receipts VALUES (?,?,?,?,?)",
                (
                    match_id,
                    seat,
                    request_id,
                    expected_revision,
                    json.dumps(result, sort_keys=True),
                ),
            )

            return result

    def cleanup_expired(self, *, limit: int) -> int:
        """Delete at most limit expired matches and their dependent records.

        Selection and deletion execute within one SQLite write transaction.
        A matching active match cannot disappear due to a concurrent
        cleanup versus authoritative write on the same database.
        """
        if type(limit) is not int or not 1 <= limit <= 1000:
            raise LifecycleAuthorityError("Invalid cleanup limit")

        with self._write() as db:
            now = self._now()
            rows = db.execute(
                "SELECT match_id FROM lifecycle_matches "
                "WHERE expires_at<=? ORDER BY expires_at,match_id LIMIT ?",
                (now, limit),
            ).fetchall()

            deleted = 0
            for (match_id,) in rows:
                result = db.execute(
                    "DELETE FROM lifecycle_matches WHERE match_id=? AND expires_at<=?",
                    (match_id, now),
                )
                deleted += result.rowcount

            return deleted
