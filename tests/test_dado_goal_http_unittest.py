"""Real loopback HTTP tests without external dependencies."""

import http.client
import json
import threading
import unittest
from typing import Any
from unittest.mock import patch

from boardlab.games.dado_goal_http import create_local_server


class HTTPTests(unittest.TestCase):
    def setUp(self) -> None:
        self.server = create_local_server()
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def tearDown(self) -> None:
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)

    def request(
        self,
        method: Any,
        path: Any,
        body: Any = None,
        token: Any = None,
        *,
        origin: Any = None,
        host: Any = None,
        content_type: Any = "application/json",
    ) -> Any:
        conn = http.client.HTTPConnection("127.0.0.1", self.server.server_port, timeout=5)
        headers = {"Content-Type": content_type}
        if token:
            headers["Authorization"] = f"Bearer {token}"
        if origin:
            headers["Origin"] = origin
        if host:
            headers["Host"] = host

        raw = None
        if body is not None:
            raw = body if isinstance(body, bytes) else json.dumps(body)

        try:
            conn.request(method, path, body=raw, headers=headers)
            response = conn.getresponse()
            payload = json.loads(response.read())
            return response.status, payload, dict(response.getheaders())
        finally:
            conn.close()

    def create(self) -> Any:
        status, created, headers = self.request("POST", "/api/v1/matches", {"handsPerTeam": 1})
        self.assertEqual(status, 201)
        self.assertEqual(headers["Cache-Control"], "no-store")
        return created

    def test_full_match_and_replay(self) -> None:
        created = self.create()
        mid = created["matchId"]
        host = created["hostToken"]

        status, waiting, _ = self.request("GET", f"/api/v1/matches/{mid}", token=host)
        self.assertEqual(status, 200)
        self.assertEqual(waiting["status"], "waiting")

        status, guest_data, _ = self.request(
            "POST", f"/api/v1/matches/{mid}/join", {"inviteToken": created["inviteToken"]}
        )
        self.assertEqual(status, 200)
        guest = guest_data["guestToken"]

        with patch(
            "boardlab.games.dado_goal_match.secrets.randbelow",
            return_value=2,
        ) as rng:
            status, first, _ = self.request(
                "POST",
                f"/api/v1/matches/{mid}/roll",
                {"requestId": "host-1", "expectedRevision": 1},
                token=host,
            )
            self.assertEqual(status, 200)
            self.assertEqual(first["revision"], 2)

            status, replay, _ = self.request(
                "POST",
                f"/api/v1/matches/{mid}/roll",
                {"requestId": "host-1", "expectedRevision": 1},
                token=host,
            )
            self.assertEqual(status, 200)
            self.assertTrue(replay["duplicate"])

            status, last, _ = self.request(
                "POST",
                f"/api/v1/matches/{mid}/roll",
                {"requestId": "guest-1", "expectedRevision": 2},
                token=guest,
            )

        self.assertEqual(rng.call_count, 2)
        self.assertEqual(status, 200)
        self.assertEqual(last["status"], "completed")
        self.assertEqual(last["state"]["phase"], "finished")

        for seat in (host, guest):
            status, snapshot, _ = self.request("GET", f"/api/v1/matches/{mid}", token=seat)
            self.assertEqual(status, 200)
            self.assertEqual(snapshot["state"], last["state"])
            self.assertEqual(snapshot["revision"], 3)

    def test_authentication_and_invitation(self) -> None:
        created = self.create()
        path = f"/api/v1/matches/{created['matchId']}"

        status, data, _ = self.request("GET", path, token=created["inviteToken"])
        self.assertEqual(status, 401)
        self.assertEqual(data["error"]["code"], "UNAUTHORIZED")

        status, _, _ = self.request("POST", f"{path}/join", {"inviteToken": "wrong"})
        self.assertEqual(status, 401)

    def test_bad_input_and_origin(self) -> None:
        status, _, _ = self.request("POST", "/api/v1/matches", {"handsPerTeam": True})
        self.assertEqual(status, 400)

        status, _, _ = self.request(
            "POST", "/api/v1/matches", b'{"handsPerTeam":1,"handsPerTeam":2}'
        )
        self.assertEqual(status, 400)

        status, _, _ = self.request(
            "POST", "/api/v1/matches", {"handsPerTeam": 1}, origin="https://untrusted.example"
        )
        self.assertEqual(status, 403)

        status, _, _ = self.request(
            "POST", "/api/v1/matches", {"handsPerTeam": 1}, host="malicious.example"
        )
        self.assertEqual(status, 400)

        status, _, _ = self.request(
            "POST", "/api/v1/matches", {"handsPerTeam": 1}, content_type="text/plain"
        )
        self.assertEqual(status, 415)

    def test_wrong_turn_and_stale_revision(self) -> None:
        created = self.create()
        mid = created["matchId"]
        _, joined, _ = self.request(
            "POST", f"/api/v1/matches/{mid}/join", {"inviteToken": created["inviteToken"]}
        )

        path = f"/api/v1/matches/{mid}/roll"

        status, data, _ = self.request(
            "POST",
            path,
            {"requestId": "bad-turn", "expectedRevision": 1},
            token=joined["guestToken"],
        )
        self.assertEqual(status, 403)
        self.assertEqual(data["error"]["code"], "NOT_YOUR_TURN")

        status, data, _ = self.request(
            "POST", path, {"requestId": "stale", "expectedRevision": 0}, token=created["hostToken"]
        )
        self.assertEqual(status, 409)
        self.assertEqual(data["error"]["code"], "STALE_REVISION")


if __name__ == "__main__":
    unittest.main()
