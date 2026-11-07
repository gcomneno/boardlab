"""Verify ambiguous HTTP framing is rejected and connections close."""

import socket
import threading
import unittest

from boardlab.games.dado_goal_http import create_local_server


class FramingTests(unittest.TestCase):
    def setUp(self):
        self.server = create_local_server()
        self.thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)

    def raw_request(self, request):
        with socket.create_connection(
            ("127.0.0.1", self.server.server_port),
            timeout=3,
        ) as connection:
            connection.settimeout(3)
            connection.sendall(request)
            connection.shutdown(socket.SHUT_WR)

            chunks = []
            while True:
                block = connection.recv(8192)
                if not block:
                    break
                chunks.append(block)

        return b"".join(chunks)

    def assert_rejected_and_closed(self, request):
        response = self.raw_request(request)
        self.assertTrue(
            response.startswith(b"HTTP/1.1 400"),
            response[:300],
        )
        self.assertIn(b'"code": "BAD_REQUEST"', response)
        self.assertIn(b"Cache-Control: no-store", response)

    def test_duplicate_content_length(self):
        self.assert_rejected_and_closed(
            b"POST /api/v1/matches HTTP/1.1\r\n"
            + f"Host: 127.0.0.1:{self.server.server_port}\r\n".encode()
            + b"Content-Type: application/json\r\n"
            + b"Content-Length: 18\r\n"
            + b"Content-Length: 19\r\n"
            + b"\r\n"
            + b'{"handsPerTeam":1}'
        )

    def test_transfer_encoding_content_length_conflict(self):
        self.assert_rejected_and_closed(
            b"POST /api/v1/matches HTTP/1.1\r\n"
            + f"Host: 127.0.0.1:{self.server.server_port}\r\n".encode()
            + b"Transfer-Encoding: chunked\r\n"
            + b"Content-Length: 18\r\n"
            + b"Content-Type: application/json\r\n"
            + b"\r\n"
            + b'{"handsPerTeam":1}'
        )

    def test_transfer_encoding_alone(self):
        self.assert_rejected_and_closed(
            b"POST /api/v1/matches HTTP/1.1\r\n"
            + f"Host: 127.0.0.1:{self.server.server_port}\r\n".encode()
            + b"Transfer-Encoding: chunked\r\n"
            + b"\r\n0\r\n\r\n"
        )

    def test_get_with_body_framing(self):
        self.assert_rejected_and_closed(
            b"GET /api/v1/unknown HTTP/1.1\r\n"
            + f"Host: 127.0.0.1:{self.server.server_port}\r\n".encode()
            + b"Content-Length: 4\r\n"
            + b"\r\nTEST"
        )


if __name__ == "__main__":
    unittest.main()
