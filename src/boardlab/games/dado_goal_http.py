"""Local HTTP adapter for Dado Goal multiplayer.

Laboratory only: loopback binding, in-memory matches,
no persistent storage, expiry, rate limiting or public hosting.
"""

from __future__ import annotations

import json
import re
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import urlsplit

from boardlab.games.dado_goal_durable import (
    DurableMatchError,
    DurableMatchManager,
)
from boardlab.games.dado_goal_lifecycle import LifecyclePolicy
from boardlab.games.dado_goal_lifecycle_authority import (
    LifecycleAuthorityError,
    LifecycleAuthorityV4,
)
from boardlab.games.dado_goal_match import MatchError, MatchManager
from boardlab.games.dado_goal_rate_limit import RateLimiter

MAX_BODY = 4096
MATCH_PATH = re.compile(r"^/api/v1/matches/([A-Za-z0-9_-]{20,128})(?:/(join|roll))?$")

ERRORS = {
    "Unknown match": (404, "MATCH_NOT_FOUND"),
    "Match expired": (410, "MATCH_EXPIRED"),
    "Unauthorized participant": (401, "UNAUTHORIZED"),
    "Invalid invitation": (401, "UNAUTHORIZED"),
    "Not your turn": (403, "NOT_YOUR_TURN"),
    "Stale revision": (409, "STALE_REVISION"),
    "Match is not joinable": (409, "MATCH_NOT_JOINABLE"),
    "Match is not active": (409, "MATCH_NOT_ACTIVE"),
    "Request ID reused with other revision": (409, "REQUEST_ID_CONFLICT"),
    "Match capacity reached": (503, "CAPACITY_EXCEEDED"),
    "Invalid match length": (400, "BAD_REQUEST"),
    "Invalid request ID": (400, "BAD_REQUEST"),
    "Invalid revision": (400, "BAD_REQUEST"),
    "Invalid credential": (401, "UNAUTHORIZED"),
}


class APIError(Exception):
    def __init__(self, status: int, code: str):
        self.status = status
        self.code = code
        super().__init__(code)


def _unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    obj: dict[str, Any] = {}
    for key, value in pairs:
        if key in obj:
            raise ValueError("Duplicate JSON field")
        obj[key] = value
    return obj


def _bad_constant(value: str) -> None:
    raise ValueError("Non-finite JSON number")


class LocalHTTPServer(ThreadingHTTPServer):
    allow_reuse_address = True
    daemon_threads = True
    request_timeout_seconds: float = 5.0

    def __init__(
        self,
        manager: MatchManager | DurableMatchManager | LifecycleAuthorityV4 | None = None,
    ):
        super().__init__(("127.0.0.1", 0), DadoGoalHandler)
        self.manager = manager if manager is not None else MatchManager()
        self.rate_limiter: RateLimiter | None = None


class DadoGoalHandler(BaseHTTPRequestHandler):
    server: LocalHTTPServer
    protocol_version = "HTTP/1.1"

    def setup(self) -> None:
        super().setup()
        # Bound idle headers and incomplete HTTP request bodies.
        self.connection.settimeout(self.server.request_timeout_seconds)

    def log_message(self, format: str, *args: Any) -> None:
        # Deliberately do not log paths, tokens or request data.
        pass

    def _send(self, status: int, payload: dict[str, Any]) -> None:
        data = json.dumps(payload, ensure_ascii=False, allow_nan=False).encode("utf-8")

        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(data)

    def _error(self, status: int, code: str) -> None:
        # Never reuse an errored connection with unread input.
        self.close_connection = True
        self._send(
            status,
            {
                "error": {
                    "code": code,
                    "message": code.replace("_", " ").capitalize(),
                }
            },
        )

    def _guard(self) -> None:
        # Reject ambiguous HTTP message framing.
        if self.headers.get("Transfer-Encoding") is not None:
            raise APIError(400, "BAD_REQUEST")

        lengths = self.headers.get_all("Content-Length", [])
        if len(lengths) > 1:
            raise APIError(400, "BAD_REQUEST")

        if self.command == "GET" and lengths:
            raise APIError(400, "BAD_REQUEST")

        host = self.headers.get("Host", "")
        expected = f"127.0.0.1:{self.server.server_port}"
        if host != expected:
            raise APIError(400, "BAD_HOST")

        origin = self.headers.get("Origin")
        if origin is not None and origin != f"http://{expected}":
            raise APIError(403, "ORIGIN_DENIED")

    def _bearer(self) -> str:
        value = self.headers.get("Authorization", "")
        if not value.startswith("Bearer "):
            raise APIError(401, "UNAUTHORIZED")
        token = value[7:]
        if not 1 <= len(token) <= 256 or token.strip() != token:
            raise APIError(401, "UNAUTHORIZED")
        return token

    def _json(self) -> dict[str, Any]:
        media = self.headers.get("Content-Type", "").split(";", 1)[0]
        if media.lower().strip() != "application/json":
            raise APIError(415, "UNSUPPORTED_MEDIA_TYPE")

        sizes = self.headers.get_all("Content-Length", [])
        if len(sizes) != 1 or not sizes[0].isdigit():
            raise APIError(400, "BAD_REQUEST")

        size = int(sizes[0])
        if size > MAX_BODY:
            self.close_connection = True
            raise APIError(413, "PAYLOAD_TOO_LARGE")

        try:
            raw = self.rfile.read(size).decode("utf-8", "strict")
            obj = json.loads(
                raw,
                object_pairs_hook=_unique_object,
                parse_constant=_bad_constant,
            )
        except (UnicodeError, ValueError):
            raise APIError(400, "BAD_REQUEST") from None

        if type(obj) is not dict:
            raise APIError(400, "BAD_REQUEST")
        return obj

    @staticmethod
    def _fields(obj: dict[str, Any], expected: set[str]) -> None:
        if set(obj) != expected:
            raise APIError(400, "BAD_REQUEST")

    def _path(self) -> str:
        parsed = urlsplit(self.path)
        if parsed.query or parsed.fragment:
            raise APIError(400, "BAD_REQUEST")
        return parsed.path

    def _serve_asset(self, route: str) -> None:
        """Serve only explicitly approved multiplayer assets."""
        assets = {
            "/multiplayer.html": ("multiplayer.html", "text/html; charset=utf-8"),
            "/multiplayer-play.mjs": ("multiplayer-play.mjs", "text/javascript; charset=utf-8"),
            "/multiplayer-client.mjs": ("multiplayer-client.mjs", "text/javascript; charset=utf-8"),
            "/multiplayer-theme.css": ("multiplayer-theme.css", "text/css; charset=utf-8"),
            "/dado-goal-handmade-dice.png": ("dado-goal-handmade-dice.png", "image/png"),
        }

        filename, media_type = assets[route]
        root = Path(__file__).resolve().parents[3]
        data = (root / "web" / "dado-goal" / filename).read_bytes()

        self.send_response(200)
        self.send_header("Content-Type", media_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header(
            "Content-Security-Policy",
            "default-src 'none'; "
            "script-src 'self'; connect-src 'self'; "
            "style-src 'self'; img-src 'self'; form-action 'none'; "
            "base-uri 'none'; frame-ancestors 'none'",
        )
        self.end_headers()
        self.wfile.write(data)

    def _dispatch(self) -> None:
        self._guard()
        path = self._path()
        if self.command == "POST" and (
            path == "/api/v1/matches"
            or (
                (match := MATCH_PATH.fullmatch(path)) is not None
                and match.group(2) in ("join", "roll")
            )
        ):
            limiter = self.server.rate_limiter
            if limiter is not None and not limiter.allow():
                raise APIError(429, "RATE_LIMITED")

        if self.command == "GET" and path in {
            "/multiplayer.html",
            "/multiplayer-play.mjs",
            "/multiplayer-client.mjs",
            "/multiplayer-theme.css",
            "/dado-goal-handmade-dice.png",
        }:
            self._serve_asset(path)
            return

        match = MATCH_PATH.fullmatch(path)

        if self.command == "POST" and path == "/api/v1/matches":
            body = self._json()
            self._fields(body, {"handsPerTeam"})
            self._send(201, self.server.manager.create(body["handsPerTeam"]))
            return

        if match is None:
            raise APIError(404, "NOT_FOUND")

        match_id, action = match.groups()

        if self.command == "GET" and action is None:
            self._send(200, self.server.manager.snapshot(match_id, self._bearer()))
            return

        if self.command == "POST" and action == "join":
            body = self._json()
            self._fields(body, {"inviteToken"})
            self._send(
                200,
                self.server.manager.join(match_id, body["inviteToken"]),
            )
            return

        if self.command == "POST" and action == "roll":
            token = self._bearer()
            body = self._json()
            self._fields(body, {"requestId", "expectedRevision"})
            self._send(
                200,
                self.server.manager.roll(
                    match_id,
                    token,
                    body["requestId"],
                    body["expectedRevision"],
                ),
            )
            return

        raise APIError(404, "NOT_FOUND")

    def _handle(self) -> None:
        try:
            self._dispatch()
        except APIError as exc:
            self._error(exc.status, exc.code)
        except (MatchError, DurableMatchError, LifecycleAuthorityError) as exc:
            status, code = ERRORS.get(str(exc), (400, "BAD_REQUEST"))
            self._error(status, code)

    def do_GET(self) -> None:
        self._handle()

    def do_POST(self) -> None:
        self._handle()


def create_local_server(
    manager: MatchManager | DurableMatchManager | LifecycleAuthorityV4 | None = None,
) -> LocalHTTPServer:
    """Construct a loopback-only server; caller controls its lifecycle."""
    return LocalHTTPServer(manager)


def create_durable_local_server(
    database_path: str | Path,
) -> LocalHTTPServer:
    """Explicit opt-in SQLite server, still bound to loopback.

    Initializing an existing incompatible database fails closed.
    The default in-memory factory remains unchanged.
    """
    manager = DurableMatchManager(database_path)
    manager.initialize()
    return create_local_server(manager)


def create_lifecycle_local_server(
    database_path: str | Path,
    policy: LifecyclePolicy,
) -> LocalHTTPServer:
    """Explicit SQLite v4 HTTP factory; loopback only."""
    manager = LifecycleAuthorityV4(database_path, policy)
    manager.initialize()
    return create_local_server(manager)


def create_rate_limited_local_server(
    manager: MatchManager | DurableMatchManager | LifecycleAuthorityV4,
    limiter: RateLimiter,
) -> LocalHTTPServer:
    """Explicit opt-in rate limiting for loopback HTTP tests."""
    if not isinstance(limiter, RateLimiter):
        raise TypeError("RateLimiter required")
    server = create_local_server(manager)
    server.rate_limiter = limiter
    return server
