"""Experimental SQLite v2 credentials for Dado Goal.

Not connected to MatchManager or HTTP.
No database is created during import.
Do not expose raw credentials through snapshots.
"""

from __future__ import annotations

import hashlib
import hmac
import json
import secrets
import sqlite3
from pathlib import Path

from boardlab.games.dado_goal import initial_state
from boardlab.games.dado_goal_wire import encode_state


class CredentialError(ValueError):
    pass


def _digest(token: str) -> bytes:
    if type(token) is not str or not 1 <= len(token) <= 256:
        raise CredentialError("Invalid credential")
    return hashlib.sha256(token.encode("utf-8")).digest()


def _new_token() -> str:
    return secrets.token_urlsafe(32)


class DurableCredentials:
    """Single-file transactional credential laboratory."""

    def __init__(self, path: str | Path):
        self.path = Path(path)

        if str(path) == ":memory:" or not self.path.parent.is_dir():
            raise ValueError("Explicit filesystem database path required")

    def _connect(self) -> sqlite3.Connection:
        db = sqlite3.connect(
            self.path,
            timeout=5,
            isolation_level=None,
        )
        db.execute("PRAGMA foreign_keys=ON")
        db.execute("PRAGMA busy_timeout=5000")
        return db

    def initialize(self) -> None:
        """Initialize only a new database; never silently migrate v1."""

        if self.path.exists():
            with self._connect() as db:
                version = db.execute("PRAGMA user_version").fetchone()[0]

                if version != 2:
                    raise CredentialError(
                        "Unsupported schema: explicit migration required"
                    )

                required = {"credential_matches", "credential_seats"}
                found = {
                    row[0] for row in db.execute(
                        "SELECT name FROM sqlite_master "
                        "WHERE type='table'"
                    )
                }

                if not required.issubset(found):
                    raise CredentialError("Incomplete credential schema")

            return

        with self._connect() as db:
            try:
                db.execute("BEGIN IMMEDIATE")
                db.execute("""
                    CREATE TABLE credential_matches (
                        match_id TEXT PRIMARY KEY,
                        status TEXT NOT NULL
                            CHECK(status IN ('waiting','active')),
                        revision INTEGER NOT NULL
                            CHECK(revision >= 0),
                        state_json TEXT NOT NULL,
                        invite_digest BLOB,
                        invite_used INTEGER NOT NULL DEFAULT 0
                            CHECK(invite_used IN (0,1))
                    )
                """)
                db.execute("""
                    CREATE TABLE credential_seats (
                        match_id TEXT NOT NULL
                            REFERENCES credential_matches(match_id),
                        seat INTEGER NOT NULL CHECK(seat IN (0,1)),
                        token_digest BLOB NOT NULL,
                        PRIMARY KEY(match_id,seat),
                        UNIQUE(token_digest)
                    )
                """)
                db.execute("PRAGMA user_version=2")
                db.commit()
            except BaseException:
                db.rollback()
                raise

    def create(self, hands_per_team: int) -> dict[str, str]:
        if (
            type(hands_per_team) is not int
            or not 1 <= hands_per_team <= 1000
        ):
            raise CredentialError("Invalid match length")

        match_id = _new_token()
        host = _new_token()
        invite = _new_token()
        state = json.dumps(
            encode_state(initial_state(hands_per_team)),
            sort_keys=True,
        )

        with self._connect() as db:
            try:
                db.execute("BEGIN IMMEDIATE")
                db.execute(
                    "INSERT INTO credential_matches VALUES "
                    "(?, 'waiting', 0, ?, ?, 0)",
                    (match_id, state, _digest(invite)),
                )
                db.execute(
                    "INSERT INTO credential_seats VALUES (?, 0, ?)",
                    (match_id, _digest(host)),
                )
                db.commit()
            except BaseException:
                db.rollback()
                raise

        return {
            "matchId": match_id,
            "hostToken": host,
            "inviteToken": invite,
        }

    def join(self, match_id: str, invitation: str) -> str:
        provided = _digest(invitation)

        with self._connect() as db:
            try:
                db.execute("BEGIN IMMEDIATE")
                row = db.execute(
                    "SELECT status, invite_digest, invite_used "
                    "FROM credential_matches WHERE match_id=?",
                    (match_id,),
                ).fetchone()

                if row is None or row[0] != "waiting" or row[2]:
                    raise CredentialError("Match not joinable")

                if row[1] is None or not hmac.compare_digest(
                    bytes(row[1]), provided
                ):
                    raise CredentialError("Invalid invitation")

                guest = _new_token()

                db.execute(
                    "INSERT INTO credential_seats VALUES (?, 1, ?)",
                    (match_id, _digest(guest)),
                )

                changed = db.execute(
                    "UPDATE credential_matches SET "
                    "status='active', revision=revision+1, "
                    "invite_used=1, invite_digest=NULL "
                    "WHERE match_id=? AND status='waiting' "
                    "AND invite_used=0",
                    (match_id,),
                ).rowcount

                if changed != 1:
                    raise CredentialError("Join conflict")

                db.commit()
                return guest
            except BaseException:
                db.rollback()
                raise

    def verify_seat(self, match_id: str, seat: int, token: str) -> bool:
        if type(seat) is not int or seat not in (0, 1):
            raise CredentialError("Invalid seat")

        digest = _digest(token)

        with self._connect() as db:
            row = db.execute(
                "SELECT token_digest FROM credential_seats "
                "WHERE match_id=? AND seat=?",
                (match_id, seat),
            ).fetchone()

        return row is not None and hmac.compare_digest(
            bytes(row[0]), digest
        )

    def snapshot(self, match_id: str) -> dict[str, object]:
        with self._connect() as db:
            row = db.execute(
                "SELECT status, revision, state_json "
                "FROM credential_matches WHERE match_id=?",
                (match_id,),
            ).fetchone()

        if row is None:
            raise CredentialError("Unknown match")

        return {
            "status": row[0],
            "revision": row[1],
            "state": json.loads(row[2]),
        }
