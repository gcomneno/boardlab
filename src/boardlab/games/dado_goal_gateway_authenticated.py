"""Opt-in browser authentication for the Dado Goal gateway.

Loopback only. Public deployment remains prohibited.
"""

from __future__ import annotations

import json
from contextlib import suppress
from typing import Any

from boardlab.games.dado_goal_gateway import (
    GatewayHandler,
    GatewayServer,
)
from boardlab.games.dado_goal_gateway_browser import BrowserAuthentication
from boardlab.games.dado_goal_gateway_sessions import GatewaySessions
from boardlab.games.dado_goal_rate_limit import RateLimiter


class AuthenticatedGatewayServer(GatewayServer):
    def __init__(
        self,
        upstream_port: int,
        *,
        access_key: str,
        public_origin: str | None = None,
    ) -> None:
        super().__init__(upstream_port, public_origin=public_origin)
        try:
            self.browser_auth = BrowserAuthentication(
                access_key=access_key,
                sessions=GatewaySessions(),
            )
            self.login_limiter = RateLimiter(
                limit=10, window_seconds=60
            )
            self.RequestHandlerClass = AuthenticatedGatewayHandler
        except BaseException:
            self.server_close()
            raise


class AuthenticatedGatewayHandler(GatewayHandler):
    server: AuthenticatedGatewayServer

    def _json_reply(
        self,
        status: int,
        payload: dict[str, Any],
        *,
        cookie: str | None = None,
    ) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Connection", "close")
        if cookie is not None:
            self.send_header("Set-Cookie", cookie)
        self.end_headers()
        self.close_connection = True
        with suppress(BrokenPipeError, ConnectionResetError):
            self.wfile.write(body)

    def _single_header(self, name: str) -> str | None:
        values = self.headers.get_all(name, [])
        if len(values) != 1:
            return None
        return values[0]

    def _boundary_valid(self) -> bool:
        expected_host = (
            self.server.public_origin.split("://", 1)[1]
            if self.server.public_origin is not None
            else f"127.0.0.1:{self.server.server_port}"
        )
        expected_origin = (
            self.server.public_origin
            or f"http://{expected_host}"
        )
        if self._single_header("Host") != expected_host:
            return False

        origins = self.headers.get_all("Origin", [])
        if self.command == "GET":
            # Top-level browser navigation normally omits Origin.
            # Explicitly supplied origins must still match.
            return not origins or (
                len(origins) == 1 and origins[0] == expected_origin
            )

        # State-changing requests require one exact trusted Origin.
        return len(origins) == 1 and origins[0] == expected_origin

    def _login(self) -> None:
        if not self.server.login_limiter.allow():
            self._json_reply(429, {"error": "RATE_LIMITED"})
            return

        if self.headers.get("Transfer-Encoding") is not None:
            self._json_reply(400, {"error": "BAD_FRAMING"})
            return

        lengths = self.headers.get_all("Content-Length", [])
        if len(lengths) != 1 or not lengths[0].isdecimal():
            self._json_reply(400, {"error": "BAD_FRAMING"})
            return

        size = int(lengths[0])
        if not 0 < size <= 4096:
            self._json_reply(413, {"error": "BODY_LIMIT"})
            return

        if self._single_header("Content-Type") != "application/json":
            self._json_reply(415, {"error": "CONTENT_TYPE"})
            return

        try:
            data = json.loads(self.rfile.read(size))
        except (UnicodeDecodeError, ValueError):
            self._json_reply(400, {"error": "BAD_JSON"})
            return

        if not isinstance(data, dict) or set(data) != {"accessKey"}:
            self._json_reply(400, {"error": "BAD_REQUEST"})
            return

        try:
            session = self.server.browser_auth.login(data["accessKey"])
        except RuntimeError:
            self._json_reply(503, {"error": "SESSION_CAPACITY"})
            return

        if session is None:
            self._json_reply(403, {"error": "ACCESS_DENIED"})
            return

        self._json_reply(
            200,
            {"csrfToken": session.csrf_token},
            cookie=self.server.browser_auth.cookie(session),
        )

    def _handle(self) -> None:
        # Require exact Host and Origin before authentication.
        # For this laboratory adapter, even GET requests require Origin.
        if not self._boundary_valid():
            self._json_reply(403, {"error": "BOUNDARY_DENIED"})
            return

        if self.path == "/auth/login" and self.command == "POST":
            self._login()
            return

        public_assets = {
            "/multiplayer.html",
            "/multiplayer-play.mjs",
            "/multiplayer-client.mjs",
            "/multiplayer-theme.css",
            "/dado-goal-handmade-dice.png",
        }
        if self.command == "GET" and self.path in public_assets:
            super()._handle()
            return

        cookie = self._single_header("Cookie")
        csrf = self._single_header("X-Dado-CSRF")
        mutation = self.command != "GET"

        if not self.server.browser_auth.authenticate(
            cookie,
            csrf_header=csrf,
            mutation=mutation,
        ):
            self._json_reply(403, {"error": "SESSION_DENIED"})
            return

        if self.path == "/auth/logout" and self.command == "POST":
            self.server.browser_auth.logout(cookie)
            self._json_reply(
                200,
                {"loggedOut": True},
                cookie=(
                    "__Host-dado-session=; Path=/; Secure; "
                    "HttpOnly; SameSite=Strict; Max-Age=0"
                ),
            )
            return

        # Existing gateway retains its Host/Origin, framing,
        # upstream and route enforcement.
        super()._handle()

    def do_GET(self) -> None:
        self._handle()

    def do_POST(self) -> None:
        self._handle()
