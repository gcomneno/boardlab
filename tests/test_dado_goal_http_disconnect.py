"""Disconnected HTTP peers must not crash response handling."""

from __future__ import annotations

from unittest.mock import Mock

import pytest

from boardlab.games.dado_goal_http import DadoGoalHandler


@pytest.mark.parametrize(
    "error",
    [BrokenPipeError("peer closed"), ConnectionResetError("peer reset")],
)
def test_json_response_handles_disconnected_peer(error: OSError) -> None:
    handler = object.__new__(DadoGoalHandler)
    handler.wfile = Mock()
    handler.wfile.write.side_effect = error
    handler.close_connection = False
    handler.send_response = Mock()
    handler.send_header = Mock()
    handler.end_headers = Mock()

    handler._send(400, {"error": {"code": "BAD_HOST"}})

    assert handler.close_connection is True
    handler.send_response.assert_called_once_with(400)


@pytest.mark.parametrize(
    "error",
    [BrokenPipeError("peer closed"), ConnectionResetError("peer reset")],
)
def test_asset_response_handles_disconnected_peer(
    error: OSError,
) -> None:
    handler = object.__new__(DadoGoalHandler)
    handler.wfile = Mock()
    handler.wfile.write.side_effect = error
    handler.close_connection = False
    handler.send_response = Mock()
    handler.send_header = Mock()
    handler.end_headers = Mock()

    handler._serve_asset("/multiplayer.html")

    assert handler.close_connection is True
    handler.send_response.assert_called_once_with(200)


@pytest.mark.parametrize(
    "error",
    [BrokenPipeError("closed"), ConnectionResetError("reset")],
)
def test_request_lifecycle_handles_peer_disconnect(
    monkeypatch: pytest.MonkeyPatch,
    error: OSError,
) -> None:
    from http.server import BaseHTTPRequestHandler

    def interrupted_handle(self: BaseHTTPRequestHandler) -> None:
        raise error

    monkeypatch.setattr(
        BaseHTTPRequestHandler,
        "handle",
        interrupted_handle,
    )

    handler = object.__new__(DadoGoalHandler)
    handler.close_connection = False

    handler.handle()

    assert handler.close_connection is True
