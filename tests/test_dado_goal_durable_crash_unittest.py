"""Real subprocess termination tests for durable match recovery."""

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from boardlab.games.dado_goal_durable import DurableMatchManager

CRASH_SCRIPT = r"""
import json
import os
import sys
from unittest.mock import patch

from boardlab.games.dado_goal_durable import DurableMatchManager

payload = json.loads(sys.stdin.read())

manager = DurableMatchManager(payload["path"])
manager.initialize()

if payload["mode"] == "before_commit":
    def abort_transition(*args, **kwargs):
        os._exit(78)

    with patch(
        "boardlab.games.dado_goal_durable.apply_roll",
        side_effect=abort_transition,
    ):
        manager.roll(
            payload["matchId"],
            payload["token"],
            payload["requestId"],
            1,
        )

elif payload["mode"] == "after_commit":
    manager.roll(
        payload["matchId"],
        payload["token"],
        payload["requestId"],
        1,
    )
    os._exit(77)

else:
    sys.exit(90)
"""


class ProcessCrashTests(unittest.TestCase):

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)

        self.path = Path(self.temp.name) / "matches.sqlite3"

        self.manager = DurableMatchManager(self.path)
        self.manager.initialize()

        created = self.manager.create(1)
        self.match_id = created["matchId"]
        self.host = created["hostToken"]

        self.manager.join(
            self.match_id,
            created["inviteToken"],
        )

    def subprocess_crash(self, mode, request_id):
        payload = {
            "mode": mode,
            "path": str(self.path),
            "matchId": self.match_id,
            "token": self.host,
            "requestId": request_id,
        }

        process = subprocess.run(
            [sys.executable, "-c", CRASH_SCRIPT],
            input=json.dumps(payload),
            text=True,
            capture_output=True,
            timeout=15,
            env=os.environ.copy(),
            check=False,
        )

        expected = {
            "before_commit": 78,
            "after_commit": 77,
        }[mode]

        self.assertEqual(
            process.returncode,
            expected,
            msg=process.stderr,
        )

    def test_crash_before_commit_rolls_back(self):
        before = self.manager.snapshot(
            self.match_id, self.host
        )
        before_counts = self.manager.counts(self.match_id)

        self.subprocess_crash(
            "before_commit", "aborted-request"
        )

        recovered = DurableMatchManager(self.path)
        recovered.initialize()

        self.assertEqual(
            recovered.snapshot(self.match_id, self.host),
            before,
        )
        self.assertEqual(
            recovered.counts(self.match_id),
            before_counts,
        )

    def test_crash_after_commit_preserves_receipt(self):
        self.subprocess_crash(
            "after_commit", "committed-request"
        )

        recovered = DurableMatchManager(self.path)
        recovered.initialize()

        self.assertEqual(
            recovered.snapshot(
                self.match_id, self.host
            )["revision"],
            2,
        )

        with patch(
            "boardlab.games.dado_goal_durable.secrets.randbelow",
            side_effect=AssertionError(
                "Replay attempted another die roll"
            ),
        ):
            replay = recovered.roll(
                self.match_id,
                self.host,
                "committed-request",
                1,
            )

        self.assertTrue(replay["duplicate"])
        self.assertEqual(replay["revision"], 2)
        self.assertEqual(
            recovered.counts(self.match_id),
            (2, 1),
        )

    def test_recovered_authorization(self):
        self.subprocess_crash(
            "after_commit", "authorized-request"
        )

        recovered = DurableMatchManager(self.path)
        recovered.initialize()

        snapshot = recovered.snapshot(
            self.match_id, self.host
        )

        self.assertEqual(snapshot["seat"], 0)

    def test_database_integrity_after_crash(self):
        self.subprocess_crash(
            "before_commit", "integrity-request"
        )

        import sqlite3

        with sqlite3.connect(self.path) as db:
            result = db.execute(
                "PRAGMA integrity_check"
            ).fetchone()[0]
            self.assertEqual(result, "ok")

            foreign_keys = db.execute(
                "PRAGMA foreign_key_check"
            ).fetchall()
            self.assertEqual(foreign_keys, [])


if __name__ == "__main__":
    unittest.main()
