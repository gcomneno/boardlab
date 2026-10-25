"""Static multiplayer page served on the local API origin."""

import http.client
import threading
import unittest

from boardlab.games.dado_goal_http import create_local_server


class MultiplayerPageTests(unittest.TestCase):

    def setUp(self):
        self.server = create_local_server()
        self.thread = threading.Thread(
            target=self.server.serve_forever, daemon=True
        )
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)

    def fetch(self, path, method="GET"):
        conn = http.client.HTTPConnection(
            "127.0.0.1", self.server.server_port, timeout=5
        )
        try:
            conn.request(method, path)
            response = conn.getresponse()
            return (
                response.status,
                response.read(),
                dict(response.getheaders()),
            )
        finally:
            conn.close()

    def test_page_and_modules(self):
        for path in (
            "/multiplayer.html",
            "/multiplayer-play.mjs",
            "/multiplayer-client.mjs",
        ):
            with self.subTest(path=path):
                status, body, headers = self.fetch(path)
                self.assertEqual(status, 200)
                self.assertTrue(body)
                self.assertEqual(
                    headers["Cache-Control"], "no-store"
                )
                self.assertIn(
                    "Content-Security-Policy", headers
                )

    def test_private_files_not_served(self):
        for path in (
            "/../src/boardlab/games/dado_goal_match.py",
            "/engine.mjs",
            "/app.mjs",
            "/CHANGELOG.md",
        ):
            with self.subTest(path=path):
                status, _, _ = self.fetch(path)
                self.assertEqual(status, 404)

    def test_post_cannot_read_static_page(self):
        status, _, _ = self.fetch(
            "/multiplayer.html", method="POST"
        )
        self.assertEqual(status, 404)


if __name__ == "__main__":
    unittest.main()
