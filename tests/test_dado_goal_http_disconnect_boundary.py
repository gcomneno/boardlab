"""Expected network disconnects are quiet; unexpected faults propagate."""

from __future__ import annotations

from unittest.mock import Mock, patch

import pytest

from boardlab.games.dado_goal_http import DadoGoalHandler


@pytest.mark.parametrize(
    "error",
    [
        ConnectionResetError("peer reset"),
        BrokenPipeError("peer disconnected"),
        TimeoutError("idle socket"),
    ],
)
def test_expected_network_failures_close_connection(
    error: OSError,
) -> None:
    handler = object.__new__(DadoGoalHandler)
    handler.close_connection = False

    with patch(
        "http.server.BaseHTTPRequestHandler.handle",
        side_effect=error,
    ):
        handler.handle()

    assert handler.close_connection is True


@pytest.mark.parametrize(
    "error",
    [
        RuntimeError("unexpected application fault"),
        ValueError("unexpected state"),
    ],
)
def test_unexpected_errors_are_not_suppressed(
    error: Exception,
) -> None:
    handler = object.__new__(DadoGoalHandler)
    handler.close_connection = False

    with (
        patch(
            "http.server.BaseHTTPRequestHandler.handle",
            side_effect=error,
        ),
        pytest.raises(type(error), match="unexpected"),
    ):
        handler.handle()


def test_successful_handle_is_preserved() -> None:
    handler = object.__new__(DadoGoalHandler)
    handler.close_connection = False
    parent_handle = Mock()

    with patch(
        "http.server.BaseHTTPRequestHandler.handle",
        parent_handle,
    ):
        handler.handle()

    parent_handle.assert_called_once_with()
    assert handler.close_connection is False
