"""Local HTTP interface for L'Arca di Darwin."""

import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import urlsplit

from boardlab.games.arca_di_darwin import (
    SCENARIOS,
    SPECIES,
    evaluate_selection,
    scenario,
    validate_hold,
    viable_configurations,
)
from boardlab.games.arca_di_darwin_levels import (
    ANIMALS,
    build_level,
    check_level,
)

PAGE = Path(__file__).resolve().parents[3] / "web/arca-di-darwin/index.html"


class Handler(BaseHTTPRequestHandler):
    def respond(self, status: int, payload: dict[str, Any]) -> None:
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        path = urlsplit(self.path).path
        if path == "/api/levels":
            self.respond(
                200,
                {
                    "levels": [
                        {
                            "difficulty": difficulty,
                            "animals": list(ANIMALS),
                            "clues": list(build_level(difficulty).clues),
                        }
                        for difficulty in ("facile", "medio", "difficile")
                    ]
                },
            )
            return
        if path == "/api/species":
            self.respond(
                200,
                {
                    "species": [vars(s) for s in SPECIES],
                    "scenarios": [
                        {**s, "viable_count": len(viable_configurations(s))} for s in SCENARIOS
                    ],
                },
            )
            return
        if path == "/assets/arca-di-darwin-banner.png":
            image = PAGE.parent / "assets/arca-di-darwin-banner.png"
            if not image.is_file():
                self.send_error(404)
                return
            data = image.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        if path == "/" and PAGE.is_file():
            data = PAGE.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        self.send_error(404)

    def do_POST(self) -> None:
        path = urlsplit(self.path).path
        if path not in ("/api/validate", "/api/levels/check"):
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 4096:
                raise ValueError()
            payload = json.loads(self.rfile.read(length))
            if not isinstance(payload, dict):
                raise ValueError()
            if path == "/api/levels/check":
                difficulty = payload["difficulty"]
                arrangement = payload["arrangement"]
                if type(difficulty) is not str or difficulty not in (
                    "facile",
                    "medio",
                    "difficile",
                ):
                    raise ValueError()
                if not isinstance(arrangement, list) or not all(
                    type(name) is str for name in arrangement
                ):
                    raise ValueError()
                valid = check_level(build_level(difficulty), tuple(arrangement))
                self.respond(
                    200,
                    {
                        "valid": valid,
                        "message": (
                            "CIP CIP! Rompicapo risolto!"
                            if valid
                            else "Non ancora! Rileggi gli indizi e riprova."
                        ),
                    },
                )
                return
            names = payload["species"]
            environment = scenario(payload["scenario"])
            if environment is None or not isinstance(names, list):
                raise ValueError()
            if not all(type(name) is str for name in names):
                raise ValueError()
            arrangement = payload.get("arrangement")
            if arrangement is not None:
                if not isinstance(arrangement, list):
                    raise ValueError()
                if arrangement != names:
                    raise ValueError()
                self.respond(200, validate_hold(arrangement, environment))
            else:
                self.respond(200, evaluate_selection(names, environment))
        except (ValueError, KeyError, TypeError, UnicodeDecodeError):
            self.respond(400, {"error": "Richiesta non valida"})


def main() -> None:
    server = ThreadingHTTPServer(("127.0.0.1", 8765), Handler)
    print("L'Arca di Darwin: http://127.0.0.1:8765/", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
