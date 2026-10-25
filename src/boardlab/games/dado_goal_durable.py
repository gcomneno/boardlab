"""Experimental durable authoritative Dado Goal match manager.

Standalone SQLite schema v3. No implicit migration from
laboratory schemas v1 or v2. Not connected to HTTP.

All player authorization, dice selection, state changes
and idempotency receipts are handled by this authority.
"""

from __future__ import annotations

import hashlib
import hmac
import json
import secrets
import sqlite3
from collections.abc import Iterator
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
from boardlab.games.dado_goal_wire import decode_state, encode_state


class DurableMatchError(ValueError):
    """Rejected durable multiplayer operation."""


FACES = {
    Phase.TEAM: TEAM_FACES,
    Phase.PINK: PINK_FACES,
    Phase.RESULT: RESULT_FACES,
}

SCHEMA_VERSION = 3


def _json(value: object) -> str:
    return json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
        allow_nan=False,
    )


def _token() -> str:
    return secrets.token_urlsafe(32)


def _digest(value: str) -> bytes:
    if type(value) is not str or not 1 <= len(value) <= 256:
        raise DurableMatchError("Invalid credential")
    return hashlib.sha256(value.encode("utf-8")).digest()


class DurableMatchManager:
    """Single-database, transactionally authoritative laboratory."""

    def __init__(self, path: str | Path):
        self.path = Path(path)
        if str(self.path) == ":memory:":
            raise ValueError("Durable filesystem path required")
        if not self.path.parent.is_dir():
            raise ValueError("Database directory does not exist")

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(
            self.path,
            timeout=5,
            isolation_level=None,
        )
        connection.execute("PRAGMA foreign_keys=ON")
        connection.execute("PRAGMA busy_timeout=5000")
        return connection

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

    def initialize(self) -> None:
        """Create schema explicitly; reject existing unknown formats."""
        if self.path.exists():
            db = self._connect()
            try:
                version = db.execute("PRAGMA user_version").fetchone()[0]
                required = {
                    "durable_matches",
                    "durable_seats",
                    "durable_events",
                    "durable_receipts",
                }
                tables = {
                    row[0]
                    for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")
                }
                if version != SCHEMA_VERSION or not required <= tables:
                    raise DurableMatchError("Unsupported or incomplete database schema")
            finally:
                db.close()
            return

        with self._write() as db:
            db.execute("""
                CREATE TABLE durable_matches (
                    match_id TEXT PRIMARY KEY,
                    revision INTEGER NOT NULL CHECK(revision >= 0),
                    status TEXT NOT NULL CHECK(
                        status IN ('waiting','active','completed')
                    ),
                    state_json TEXT NOT NULL,
                    invite_digest BLOB,
                    invite_used INTEGER NOT NULL DEFAULT 0
                        CHECK(invite_used IN (0,1))
                )
            """)
            db.execute("""
                CREATE TABLE durable_seats (
                    match_id TEXT NOT NULL
                        REFERENCES durable_matches(match_id),
                    seat INTEGER NOT NULL CHECK(seat IN (0,1)),
                    token_digest BLOB NOT NULL,
                    PRIMARY KEY(match_id,seat),
                    UNIQUE(token_digest)
                )
            """)
            db.execute("""
                CREATE TABLE durable_events (
                    match_id TEXT NOT NULL
                        REFERENCES durable_matches(match_id),
                    revision INTEGER NOT NULL,
                    event_json TEXT NOT NULL,
                    PRIMARY KEY(match_id,revision)
                )
            """)
            db.execute("""
                CREATE TABLE durable_receipts (
                    match_id TEXT NOT NULL
                        REFERENCES durable_matches(match_id),
                    seat INTEGER NOT NULL CHECK(seat IN (0,1)),
                    request_id TEXT NOT NULL,
                    expected_revision INTEGER NOT NULL,
                    response_json TEXT NOT NULL,
                    PRIMARY KEY(match_id,seat,request_id)
                )
            """)
            db.execute(f"PRAGMA user_version={SCHEMA_VERSION}")

    @staticmethod
    def _seat(
        db: sqlite3.Connection,
        match_id: str,
        token: str,
    ) -> int:
        digest = _digest(token)
        rows = db.execute(
            "SELECT seat,token_digest FROM durable_seats WHERE match_id=? ORDER BY seat",
            (match_id,),
        ).fetchall()

        for seat, stored in rows:
            if hmac.compare_digest(bytes(stored), digest):
                return int(seat)

        raise DurableMatchError("Unauthorized participant")

    @staticmethod
    def _read_match(
        db: sqlite3.Connection,
        match_id: str,
    ) -> tuple[int, str, Any]:
        row = db.execute(
            "SELECT revision,status,state_json FROM durable_matches WHERE match_id=?",
            (match_id,),
        ).fetchone()

        if row is None:
            raise DurableMatchError("Unknown match")

        revision, status, payload = row
        return revision, status, decode_state(json.loads(payload))

    @staticmethod
    def _snapshot(
        revision: int,
        status: str,
        state: Any,
        seat: int,
    ) -> dict[str, Any]:
        return {
            "revision": revision,
            "status": status,
            "seat": seat,
            "state": encode_state(state),
        }

    def create(self, hands_per_team: int) -> dict[str, Any]:
        if type(hands_per_team) is not int or not 1 <= hands_per_team <= 1000:
            raise DurableMatchError("Invalid match length")

        match_id = _token()
        host = _token()
        invite = _token()
        state = initial_state(hands_per_team)

        with self._write() as db:
            db.execute(
                "INSERT INTO durable_matches "
                "(match_id,revision,status,state_json,"
                "invite_digest,invite_used) "
                "VALUES (?,0,'waiting',?,?,0)",
                (
                    match_id,
                    _json(encode_state(state)),
                    _digest(invite),
                ),
            )
            db.execute(
                "INSERT INTO durable_seats VALUES (?,0,?)",
                (match_id, _digest(host)),
            )

        return {
            "matchId": match_id,
            "hostToken": host,
            "inviteToken": invite,
            "snapshot": self._snapshot(0, "waiting", state, 0),
        }

    def join(
        self,
        match_id: str,
        invite_token: str,
    ) -> dict[str, Any]:
        candidate = _digest(invite_token)

        with self._write() as db:
            row = db.execute(
                "SELECT revision,status,invite_digest,invite_used,"
                "state_json FROM durable_matches WHERE match_id=?",
                (match_id,),
            ).fetchone()

            if row is None or row[1] != "waiting" or row[3]:
                raise DurableMatchError("Match is not joinable")

            if row[2] is None or not hmac.compare_digest(bytes(row[2]), candidate):
                raise DurableMatchError("Invalid invitation")

            guest = _token()
            db.execute(
                "INSERT INTO durable_seats VALUES (?,1,?)",
                (match_id, _digest(guest)),
            )
            db.execute(
                "UPDATE durable_matches SET status='active',"
                "revision=revision+1,invite_used=1,"
                "invite_digest=NULL WHERE match_id=?",
                (match_id,),
            )
            revision = row[0] + 1
            db.execute(
                "INSERT INTO durable_events VALUES (?,?,?)",
                (match_id, revision, _json({"type": "joined"})),
            )
            state = decode_state(json.loads(row[4]))

        return {
            "guestToken": guest,
            "snapshot": self._snapshot(revision, "active", state, 1),
        }

    def snapshot(
        self,
        match_id: str,
        seat_token: str,
    ) -> dict[str, Any]:
        db = self._connect()
        try:
            seat = self._seat(db, match_id, seat_token)
            revision, status, state = self._read_match(db, match_id)
            return self._snapshot(revision, status, state, seat)
        finally:
            db.close()

    def roll(
        self,
        match_id: str,
        seat_token: str,
        request_id: str,
        expected_revision: int,
    ) -> dict[str, Any]:
        if type(request_id) is not str or not 1 <= len(request_id) <= 128:
            raise DurableMatchError("Invalid request ID")
        if type(expected_revision) is not int or expected_revision < 0:
            raise DurableMatchError("Invalid revision")

        with self._write() as db:
            seat = self._seat(db, match_id, seat_token)

            receipt = db.execute(
                "SELECT expected_revision,response_json "
                "FROM durable_receipts WHERE "
                "match_id=? AND seat=? AND request_id=?",
                (match_id, seat, request_id),
            ).fetchone()

            if receipt is not None:
                if receipt[0] != expected_revision:
                    raise DurableMatchError("Request ID reused with other revision")
                return {
                    **json.loads(receipt[1]),
                    "duplicate": True,
                }

            revision, status, state = self._read_match(db, match_id)

            if status != "active":
                raise DurableMatchError("Match is not active")
            if revision != expected_revision:
                raise DurableMatchError("Stale revision")
            if state.active_player != seat:
                raise DurableMatchError("Not your turn")

            faces = FACES.get(state.phase)
            if faces is None:
                raise DurableMatchError("No roll permitted")

            phase = state.phase
            face = faces[secrets.randbelow(len(faces))]
            assert isinstance(face, (TeamFace, PinkFace, ResultFace))
            next_state = apply_roll(state, face)
            next_revision = revision + 1
            next_status = "completed" if next_state.phase is Phase.FINISHED else "active"

            result = {
                "revision": next_revision,
                "die": phase.value,
                "face": face.value,
                "status": next_status,
                "state": encode_state(next_state),
                "duplicate": False,
            }

            updated = db.execute(
                "UPDATE durable_matches SET "
                "revision=?,status=?,state_json=? "
                "WHERE match_id=? AND revision=?",
                (
                    next_revision,
                    next_status,
                    _json(encode_state(next_state)),
                    match_id,
                    revision,
                ),
            ).rowcount

            if updated != 1:
                raise DurableMatchError("Revision conflict")

            db.execute(
                "INSERT INTO durable_events VALUES (?,?,?)",
                (
                    match_id,
                    next_revision,
                    _json(
                        {
                            "type": "roll",
                            "seat": seat,
                            "die": phase.value,
                            "face": face.value,
                        }
                    ),
                ),
            )
            db.execute(
                "INSERT INTO durable_receipts VALUES (?,?,?,?,?)",
                (
                    match_id,
                    seat,
                    request_id,
                    expected_revision,
                    _json(result),
                ),
            )

            return result

    def counts(self, match_id: str) -> tuple[int, int]:
        db = self._connect()
        try:
            events = db.execute(
                "SELECT COUNT(*) FROM durable_events WHERE match_id=?",
                (match_id,),
            ).fetchone()[0]
            receipts = db.execute(
                "SELECT COUNT(*) FROM durable_receipts WHERE match_id=?",
                (match_id,),
            ).fetchone()[0]
            return events, receipts
        finally:
            db.close()
