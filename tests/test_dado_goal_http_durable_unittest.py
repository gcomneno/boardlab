"""HTTP compatibility and restart recovery with SQLite v3."""

import http.client
import json
import tempfile
import threading
import unittest
from pathlib import Path
from typing import Any
from unittest.mock import patch

from boardlab.games.dado_goal_http import (
    create_durable_local_server,
    create_local_server,
)
from boardlab.games.dado_goal_match import MatchManager


class DurableHTTPTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "match.sqlite3"
        self.server: Any = None
        self.thread: Any = None
        self.start()

    def start(self) -> None:
        self.server = create_durable_local_server(self.path)
        self.thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )
        self.thread.start()

    def stop(self) -> None:
        if self.server is not None:
            self.server.shutdown()
            self.server.server_close()
            self.thread.join(timeout=5)
            self.server = None
            self.thread = None

    def tearDown(self) -> None:
        self.stop()

    def request(self, method: Any, path: Any, body: Any = None, token: Any = None) -> Any:
        assert self.server is not None

        connection = http.client.HTTPConnection(
            "127.0.0.1",
            self.server.server_port,
            timeout=5,
        )
        headers = {}
        if body is not None:
            headers["Content-Type"] = "application/json"
        if token is not None:
            headers["Authorization"] = f"Bearer {token}"

        try:
            connection.request(
                method,
                path,
                body=None if body is None else json.dumps(body),
                headers=headers,
            )
            response = connection.getresponse()
            return response.status, json.loads(response.read().decode("utf-8"))
        finally:
            connection.close()

    def create_and_join(self) -> Any:
        status, created = self.request("POST", "/api/v1/matches", {"handsPerTeam": 1})
        self.assertEqual(status, 201)

        match_id = created["matchId"]
        path = f"/api/v1/matches/{match_id}"

        status, guest = self.request(
            "POST",
            path + "/join",
            {"inviteToken": created["inviteToken"]},
        )
        self.assertEqual(status, 200)
        return created, guest, path

    def test_default_factory_remains_memory_based(self) -> None:
        server = create_local_server()
        try:
            self.assertIsInstance(server.manager, MatchManager)
            self.assertEqual(server.server_address[0], "127.0.0.1")
        finally:
            server.server_close()

    def test_recover_authenticated_seats_after_restart(self) -> None:
        created, guest, path = self.create_and_join()

        self.stop()
        self.start()

        for token, seat in (
            (created["hostToken"], 0),
            (guest["guestToken"], 1),
        ):
            with self.subTest(seat=seat):
                status, snapshot = self.request("GET", path, token=token)
                self.assertEqual(status, 200)
                self.assertEqual(snapshot["seat"], seat)
                self.assertEqual(snapshot["revision"], 1)
                self.assertEqual(snapshot["status"], "active")

    def test_roll_replay_after_server_restart(self) -> None:
        created, guest, path = self.create_and_join()

        with patch(
            "boardlab.games.dado_goal_durable.secrets.randbelow",
            return_value=2,
        ) as rng:
            status, first = self.request(
                "POST",
                path + "/roll",
                {"requestId": "fixed-1", "expectedRevision": 1},
                created["hostToken"],
            )
            self.assertEqual(status, 200)
            self.assertEqual(first["revision"], 2)

            self.stop()
            self.start()

            status, replay = self.request(
                "POST",
                path + "/roll",
                {"requestId": "fixed-1", "expectedRevision": 1},
                created["hostToken"],
            )
            self.assertEqual(status, 200)
            self.assertEqual(replay["revision"], 2)
            self.assertTrue(replay["duplicate"])
            self.assertEqual(first["state"], replay["state"])
            rng.assert_called_once()

        status, completed = self.request(
            "POST",
            path + "/roll",
            {"requestId": "fixed-2", "expectedRevision": 2},
            guest["guestToken"],
        )
        self.assertEqual(status, 200)
        self.assertEqual(completed["revision"], 3)

    def test_invitation_cannot_authenticate(self) -> None:
        created, _, path = self.create_and_join()

        status, error = self.request(
            "GET",
            path,
            token=created["inviteToken"],
        )
        self.assertEqual(status, 401)
        self.assertEqual(error["error"]["code"], "UNAUTHORIZED")

    def test_out_of_turn_rejected(self) -> None:
        _, guest, path = self.create_and_join()

        status, error = self.request(
            "POST",
            path + "/roll",
            {"requestId": "early", "expectedRevision": 1},
            guest["guestToken"],
        )
        self.assertEqual(status, 403)
        self.assertEqual(error["error"]["code"], "NOT_YOUR_TURN")

    def test_legacy_schema_rejected_without_mutation(self) -> None:
        self.stop()
        import sqlite3

        with sqlite3.connect(self.path) as db:
            db.execute("PRAGMA user_version=2")

        with self.assertRaises(ValueError):
            create_durable_local_server(self.path)


if __name__ == "__main__":
    unittest.main()
