"""Loopback-only gateway prototype for Dado Goal.

Not an Internet ingress. No TLS termination or proxy-header trust.
"""

from __future__ import annotations

import hmac
import http.client
from contextlib import suppress
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any
from urllib.parse import urlsplit

MAX_BODY = 65536
ALLOWED_METHODS = frozenset({"GET", "POST"})
ALLOWED_REQUEST_HEADERS = frozenset({"content-type", "authorization"})


class GatewayServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(
        self, upstream_port: int, *,
        public_origin: str | None = None,
        access_key: str | None = None,
    ) -> None:
        if type(upstream_port) is not int or not 1 <= upstream_port <= 65535:
            raise ValueError("Invalid upstream port")
        super().__init__(("127.0.0.1", 0), GatewayHandler)
        if public_origin is not None:
            parsed = urlsplit(public_origin)
            hostname = parsed.hostname
            valid_hostname = (
                hostname == "localhost"
                or (
                    hostname is not None
                    and hostname.endswith(".trycloudflare.com")
                    and len(hostname) > len(".trycloudflare.com")
                )
            )
            canonical = (
                f"https://localhost:{self.server_port}"
                if hostname == "localhost"
                else f"https://{hostname}"
            )
            if (
                not valid_hostname
                or parsed.scheme != "https"
                or parsed.path not in ("", "/")
                or parsed.query
                or parsed.fragment
                or parsed.username is not None
                or parsed.password is not None
                or public_origin != canonical
            ):
                self.server_close()
                raise ValueError("Invalid HTTPS loopback origin")
        if access_key is not None and (
            not isinstance(access_key, str)
            or not 32 <= len(access_key) <= 256
            or not access_key.isascii()
            or not access_key.isprintable()
        ):
            self.server_close()
            raise ValueError("Invalid gateway access key")
        self.access_key = access_key
        self.public_origin = public_origin
        self.upstream_port = upstream_port


class GatewayHandler(BaseHTTPRequestHandler):
    server: GatewayServer
    protocol_version = "HTTP/1.1"

    def log_message(self, format: str, *args: Any) -> None:
        pass

    def _reply(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Connection", "close")
        self.end_headers()
        self.close_connection = True
        with suppress(BrokenPipeError, ConnectionResetError):
            self.wfile.write(body)

    def _handle(self) -> None:
        expected = (
            urlsplit(self.server.public_origin).netloc
            if self.server.public_origin is not None
            else f"127.0.0.1:{self.server.server_port}"
        )

        if self.headers.get("Host") != expected:
            self._reply(400, b'{"error":"BAD_HOST"}', "application/json")
            return

        origin = self.headers.get("Origin")
        allowed_origin = self.server.public_origin or f"http://{expected}"
        if origin is not None and origin != allowed_origin:
            self._reply(403, b'{"error":"ORIGIN_DENIED"}', "application/json")
            return

        if self.command not in ALLOWED_METHODS:
            self._reply(405, b'{"error":"METHOD_DENIED"}', "application/json")
            return

        access_key = self.server.access_key
        if access_key is not None:
            supplied = self.headers.get_all("X-Dado-Access-Key", [])
            if len(supplied) != 1 or not hmac.compare_digest(
                supplied[0], access_key
            ):
                self._reply(
                    403,
                    b'{"error":"ACCESS_DENIED"}',
                    "application/json",
                )
                return

        route = urlsplit(self.path)
        if (
            not self.path.startswith("/")
            or self.path.startswith("//")
            or route.scheme
            or route.netloc
            or route.query
            or route.fragment
        ):
            self._reply(400, b'{"error":"BAD_PATH"}', "application/json")
            return

        if self.headers.get("Transfer-Encoding") is not None:
            self._reply(400, b'{"error":"BAD_FRAMING"}', "application/json")
            return

        lengths = self.headers.get_all("Content-Length", [])
        if len(lengths) > 1:
            self._reply(400, b'{"error":"BAD_FRAMING"}', "application/json")
            return

        length = 0
        if lengths:
            try:
                length = int(lengths[0])
            except ValueError:
                length = -1
        if length < 0 or length > MAX_BODY:
            self._reply(413, b'{"error":"BODY_LIMIT"}', "application/json")
            return

        if self.command == "GET" and length:
            self._reply(400, b'{"error":"BAD_FRAMING"}', "application/json")
            return

        body = self.rfile.read(length) if length else None
        headers = {
            "Host": f"127.0.0.1:{self.server.upstream_port}",
            "Connection": "close",
        }
        for key, value in self.headers.items():
            if key.lower() in ALLOWED_REQUEST_HEADERS:
                headers[key] = value
        if origin is not None:
            headers["Origin"] = (
                f"http://127.0.0.1:{self.server.upstream_port}"
            )

        try:
            connection = http.client.HTTPConnection(
                "127.0.0.1", self.server.upstream_port, timeout=5
            )
            try:
                connection.request(
                    self.command, self.path, body=body,
                    headers=headers,
                )
                response = connection.getresponse()
                payload = response.read(MAX_BODY + 1)
                if len(payload) > MAX_BODY:
                    self._reply(
                        502, b'{"error":"UPSTREAM_LIMIT"}',
                        "application/json",
                    )
                    return
                content_type = (
                    response.getheader("Content-Type")
                    or "application/octet-stream"
                )
                self._reply(response.status, payload, content_type)
            finally:
                connection.close()
        except (OSError, http.client.HTTPException):
            self._reply(
                502, b'{"error":"UPSTREAM_UNAVAILABLE"}',
                "application/json",
            )

    def do_GET(self) -> None:
        self._handle()

    def do_POST(self) -> None:
        self._handle()
