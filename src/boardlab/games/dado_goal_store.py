"""SQLite persistence laboratory for Dado Goal.

Not connected to HTTP or MatchManager.
No database is created during module import.
Caller remains responsible for authentication and secure dice.
"""

from __future__ import annotations

import json
import sqlite3
from pathlib import Path
from typing import Any, cast

from boardlab.games.dado_goal import (
    Phase,
    PinkFace,
    ResultFace,
    State,
    TeamFace,
    apply_roll,
)
from boardlab.games.dado_goal_wire import (
    decode_state,
    encode_state,
)


class StorageConflict(ValueError):
    """Revision, receipt or lifecycle conflict."""


FACES = {
    Phase.TEAM: TeamFace,
    Phase.PINK: PinkFace,
    Phase.RESULT: ResultFace,
}


def _json(value: object) -> str:
    return json.dumps(
        value,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
        allow_nan=False,
    )


class MatchStore:
    def __init__(self, path: str | Path):
        self.path = Path(path)
        if str(self.path) == ":memory:":
            raise ValueError("A durable filesystem path is required")
        if not self.path.parent.is_dir():
            raise ValueError("Database parent directory does not exist")

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.path, timeout=5, isolation_level=None)
        connection.execute("PRAGMA foreign_keys=ON")
        connection.execute("PRAGMA busy_timeout=5000")
        return connection

    def initialize(self) -> None:
        """Explicitly create the versioned schema."""
        with self._connect() as db:
            db.executescript("""
                BEGIN IMMEDIATE;

                CREATE TABLE IF NOT EXISTS schema_meta (
                    version INTEGER NOT NULL
                );

                CREATE TABLE IF NOT EXISTS matches (
                    match_id TEXT PRIMARY KEY,
                    revision INTEGER NOT NULL CHECK(revision >= 0),
                    status TEXT NOT NULL CHECK(
                        status IN ('waiting','active','completed')
                    ),
                    state_json TEXT NOT NULL
                );

                CREATE TABLE IF NOT EXISTS events (
                    match_id TEXT NOT NULL REFERENCES matches(match_id),
                    revision INTEGER NOT NULL,
                    event_json TEXT NOT NULL,
                    PRIMARY KEY(match_id, revision)
                );

                CREATE TABLE IF NOT EXISTS receipts (
                    match_id TEXT NOT NULL REFERENCES matches(match_id),
                    seat INTEGER NOT NULL CHECK(seat IN (0,1)),
                    request_id TEXT NOT NULL,
                    expected_revision INTEGER NOT NULL,
                    response_json TEXT NOT NULL,
                    PRIMARY KEY(match_id, seat, request_id)
                );

                COMMIT;
            """)
            version = db.execute("SELECT version FROM schema_meta").fetchall()
            if not version:
                db.execute("INSERT INTO schema_meta VALUES (1)")
            elif version != [(1,)]:
                raise StorageConflict("Unsupported schema version")

    def create(self, match_id: str, state: State) -> None:
        if not isinstance(match_id, str) or not match_id:
            raise ValueError("Invalid match identifier")

        encoded = _json(encode_state(state))

        with self._connect() as db:
            db.execute("BEGIN IMMEDIATE")
            try:
                db.execute(
                    """INSERT INTO matches
                       (match_id, revision, status, state_json)
                       VALUES (?, 0, 'waiting', ?)""",
                    (match_id, encoded),
                )
                db.commit()
            except BaseException:
                db.rollback()
                raise

    def activate(self, match_id: str) -> None:
        """Laboratory-only activation; no seat authentication."""
        with self._connect() as db:
            db.execute("BEGIN IMMEDIATE")
            try:
                changed = db.execute(
                    """UPDATE matches
                       SET status='active', revision=1
                       WHERE match_id=?
                         AND status='waiting'
                         AND revision=0""",
                    (match_id,),
                ).rowcount

                if changed != 1:
                    raise StorageConflict("Not joinable")

                db.execute(
                    """INSERT INTO events
                       (match_id, revision, event_json)
                       VALUES (?, 1, ?)""",
                    (match_id, _json({"type": "joined"})),
                )
                db.commit()
            except BaseException:
                db.rollback()
                raise

    def snapshot(self, match_id: str) -> dict[str, Any]:
        with self._connect() as db:
            row = db.execute(
                """SELECT revision, status, state_json
                   FROM matches WHERE match_id=?""",
                (match_id,),
            ).fetchone()

        if row is None:
            raise StorageConflict("Unknown match")

        revision, status, state_json = row
        state = decode_state(json.loads(state_json))

        return {
            "revision": revision,
            "status": status,
            "state": encode_state(state),
        }

    def roll(
        self,
        match_id: str,
        seat: int,
        request_id: str,
        expected_revision: int,
        face_value: str,
    ) -> dict[str, Any]:
        """Persist one validated transition and its receipt atomically.

        Laboratory API: face and seat are caller-supplied.
        MUST NOT be exposed directly through a network transport.
        """

        if type(seat) is not int or seat not in (0, 1):
            raise ValueError("Invalid seat")
        if type(request_id) is not str or not 1 <= len(request_id) <= 128:
            raise ValueError("Invalid request ID")
        if type(expected_revision) is not int or expected_revision < 0:
            raise ValueError("Invalid expected revision")

        with self._connect() as db:
            db.execute("BEGIN IMMEDIATE")
            try:
                receipt = db.execute(
                    """SELECT expected_revision, response_json
                       FROM receipts
                       WHERE match_id=? AND seat=? AND request_id=?""",
                    (match_id, seat, request_id),
                ).fetchone()

                if receipt is not None:
                    if receipt[0] != expected_revision:
                        raise StorageConflict("Request ID conflict")
                    result = json.loads(receipt[1])
                    result["duplicate"] = True
                    db.commit()
                    return cast(dict[str, Any], result)

                row = db.execute(
                    """SELECT revision, status, state_json
                       FROM matches WHERE match_id=?""",
                    (match_id,),
                ).fetchone()

                if row is None:
                    raise StorageConflict("Unknown match")

                revision, status, raw = row

                if status != "active":
                    raise StorageConflict("Match not active")
                if revision != expected_revision:
                    raise StorageConflict("Stale revision")

                state = decode_state(json.loads(raw))

                if state.active_player != seat:
                    raise StorageConflict("Wrong seat")

                die = FACES.get(state.phase)
                if die is None:
                    raise StorageConflict("Match finished")

                if type(face_value) is not str:
                    raise ValueError("Invalid face")

                try:
                    face = next(item for item in die if item.value == face_value)
                except StopIteration:
                    raise ValueError("Invalid die face") from None

                assert isinstance(face, (TeamFace, PinkFace, ResultFace))
                next_state = apply_roll(state, face)
                next_revision = revision + 1
                next_status = "completed" if next_state.phase is Phase.FINISHED else "active"

                result = {
                    "revision": next_revision,
                    "status": next_status,
                    "die": state.phase.value,
                    "face": face.value,
                    "state": encode_state(next_state),
                    "duplicate": False,
                }

                db.execute(
                    """UPDATE matches
                       SET revision=?, status=?, state_json=?
                       WHERE match_id=? AND revision=?""",
                    (
                        next_revision,
                        next_status,
                        _json(encode_state(next_state)),
                        match_id,
                        revision,
                    ),
                )

                db.execute(
                    """INSERT INTO events VALUES (?, ?, ?)""",
                    (
                        match_id,
                        next_revision,
                        _json(
                            {
                                "type": "roll",
                                "seat": seat,
                                "die": state.phase.value,
                                "face": face.value,
                            }
                        ),
                    ),
                )

                db.execute(
                    """INSERT INTO receipts VALUES (?, ?, ?, ?, ?)""",
                    (
                        match_id,
                        seat,
                        request_id,
                        expected_revision,
                        _json(result),
                    ),
                )

                db.commit()
                return result
            except BaseException:
                db.rollback()
                raise

    def counts(self, match_id: str) -> tuple[int, int]:
        with self._connect() as db:
            events = db.execute(
                "SELECT COUNT(*) FROM events WHERE match_id=?",
                (match_id,),
            ).fetchone()[0]
            receipts = db.execute(
                "SELECT COUNT(*) FROM receipts WHERE match_id=?",
                (match_id,),
            ).fetchone()[0]
        return events, receipts

    def integrity(self) -> bool:
        with self._connect() as db:
            result: bool = db.execute("PRAGMA integrity_check").fetchone()[0] == "ok"
            return result
