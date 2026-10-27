"""Browser-authenticated HTTP adapter for the local Dado Goal gateway."""

from __future__ import annotations

import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any

from boardlab.games.dado_goal_gateway_browser import BrowserAuthentication
from boardlab.games.dado_goal_gateway_sessions import GatewaySessions


class BrowserAuthServer(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, access_key: str) -> None:
        self.auth = BrowserAuthentication(
            access_key=access_key,
            sessions=GatewaySessions(),
        )
        super().__init__(("127.0.0.1", 0), BrowserAuthHandler)


class BrowserAuthHandler(BaseHTTPRequestHandler):
    server: BrowserAuthServer
    protocol_version = "HTTP/1.1"

    def log_message(self, format: str, *args: Any) -> None:
        pass

    def _reply(
        self,
        status: int,
        payload: dict[str, Any],
        cookie: str | None = None,
    ) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Connection", "close")
        if cookie is not None:
            self.send_header("Set-Cookie", cookie)
        self.end_headers()
        self.close_connection = True
        self.wfile.write(body)

    def _handle(self) -> None:
        host = f"127.0.0.1:{self.server.server_port}"
        if self.headers.get("Host") != host:
            self._reply(400, {"error": "BAD_HOST"})
            return

        origin = self.headers.get("Origin")
        if origin is not None and origin != f"http://{host}":
            self._reply(403, {"error": "ORIGIN_DENIED"})
            return

        if self.command != "POST":
            self._reply(405, {"error": "METHOD_DENIED"})
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            length = -1

        if length < 0 or length > 4096:
            self._reply(413, {"error": "BODY_LIMIT"})
            return

        if self.path == "/auth/login":
            try:
                data = json.loads(self.rfile.read(length))
            except (ValueError, UnicodeDecodeError):
                self._reply(400, {"error": "BAD_REQUEST"})
                return

            if not isinstance(data, dict):
                self._reply(400, {"error": "BAD_REQUEST"})
                return

            key = data.get("accessKey")
            if not isinstance(key, str):
                self._reply(403, {"error": "ACCESS_DENIED"})
                return
            session = self.server.auth.login(key)
            if session is None:
                self._reply(403, {"error": "ACCESS_DENIED"})
                return

            self._reply(
                200,
                {"csrfToken": session.csrf_token},
                self.server.auth.cookie(session),
            )
            return

        cookie = self.headers.get("Cookie")
        csrf = self.headers.get("X-Dado-CSRF")

        if not self.server.auth.authenticate(cookie, csrf_header=csrf, mutation=True):
            self._reply(403, {"error": "SESSION_DENIED"})
            return

        if self.path == "/auth/logout":
            self.server.auth.logout(cookie)
            self._reply(
                200,
                {"loggedOut": True},
                "__Host-dado-session=; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=0",
            )
            return

        self._reply(404, {"error": "NOT_FOUND"})

    def do_POST(self) -> None:
        self._handle()
