"""Additional security tests for loopback Dado Goal HTTP API."""

import http.client
import json
import threading
import unittest
from typing import Any

from boardlab.games.dado_goal_http import create_local_server


class HTTPSecurityTests(unittest.TestCase):
    def setUp(self) -> None:
        self.server = create_local_server()
        self.thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )
        self.thread.start()

    def tearDown(self) -> None:
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)

    def request(self, method: Any, path: Any, body: Any = None, headers: Any = None) -> Any:
        conn = http.client.HTTPConnection(
            "127.0.0.1",
            self.server.server_port,
            timeout=5,
        )
        headers = dict(headers or {})

        try:
            conn.request(method, path, body=body, headers=headers)
            response = conn.getresponse()
            raw = response.read()
            return (
                response.status,
                json.loads(raw),
                dict(response.getheaders()),
            )
        finally:
            conn.close()

    def test_loopback_binding(self) -> None:
        self.assertEqual(
            self.server.server_address[0],
            "127.0.0.1",
        )

    def test_unknown_routes(self) -> None:
        for path in (
            "/",
            "/api/v1/unknown",
            "/api/v1/matches/short",
        ):
            with self.subTest(path=path):
                status, body, _ = self.request("GET", path)
                self.assertEqual(status, 404)
                self.assertIn("error", body)

    def test_query_string_rejected(self) -> None:
        status, body, _ = self.request(
            "POST",
            "/api/v1/matches?token=secret",
            body=b'{"handsPerTeam":1}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 400)
        self.assertEqual(body["error"]["code"], "BAD_REQUEST")

    def test_duplicate_json_keys_rejected(self) -> None:
        status, body, _ = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"handsPerTeam":1,"handsPerTeam":2}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 400)
        self.assertEqual(body["error"]["code"], "BAD_REQUEST")

    def test_nonfinite_json_rejected(self) -> None:
        status, body, _ = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"handsPerTeam":NaN}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 400)
        self.assertEqual(body["error"]["code"], "BAD_REQUEST")

    def test_wrong_origin_rejected(self) -> None:
        status, body, _ = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"handsPerTeam":1}',
            headers={
                "Content-Type": "application/json",
                "Origin": "https://untrusted.example",
            },
        )
        self.assertEqual(status, 403)
        self.assertEqual(body["error"]["code"], "ORIGIN_DENIED")

    def test_oversized_json_rejected(self) -> None:
        status, body, _ = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"padding":"' + b"a" * 5000 + b'"}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 413)
        self.assertEqual(body["error"]["code"], "PAYLOAD_TOO_LARGE")

    def test_replies_are_not_cacheable(self) -> None:
        status, body, headers = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"handsPerTeam":1}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 201)
        self.assertIn("hostToken", body)
        self.assertEqual(headers["Cache-Control"], "no-store")
        self.assertEqual(
            headers["X-Content-Type-Options"],
            "nosniff",
        )

    def test_invitation_is_not_authorization(self) -> None:
        status, created, _ = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"handsPerTeam":1}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 201)

        mid = created["matchId"]

        status, body, _ = self.request(
            "GET",
            f"/api/v1/matches/{mid}",
            headers={"Authorization": f"Bearer {created['inviteToken']}"},
        )
        self.assertEqual(status, 401)
        self.assertEqual(body["error"]["code"], "UNAUTHORIZED")

    def test_unknown_json_properties_rejected(self) -> None:
        status, body, _ = self.request(
            "POST",
            "/api/v1/matches",
            body=b'{"handsPerTeam":1,"face":"GOAL!"}',
            headers={"Content-Type": "application/json"},
        )
        self.assertEqual(status, 400)
        self.assertEqual(body["error"]["code"], "BAD_REQUEST")


if __name__ == "__main__":
    unittest.main()
