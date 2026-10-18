"""Tests for the Tre Sigilli CLI presentation layer."""

import pytest

from boardlab.cli.tre_sigilli import parse_move, play_human_vs_ai, render_board
from boardlab.games.tre_sigilli import Place, Player, Slide, TreSigilli


def test_render_initial_board() -> None:
    state = TreSigilli().initial_state()

    assert render_board(state) == (
        "00 | 01 | 02 | 03\n04 | 05 | 06 | 07\n08 | 09 | 10 | 11\n12 | 13 | 14 | 15"
    )


def test_render_occupied_cells() -> None:
    game = TreSigilli()
    state = game.apply_move(game.initial_state(), Place(0))
    state = game.apply_move(state, Place(15))

    board = render_board(state)

    assert board.startswith(" N | 01 | 02 | 03")
    assert board.endswith("12 | 13 | 14 |  S")


def test_render_does_not_mutate_state() -> None:
    state = TreSigilli().initial_state()
    original = state.cells

    render_board(state)

    assert state.cells is original


def test_render_rejects_incorrect_board_size() -> None:
    state = TreSigilli().initial_state()
    malformed = type(state)(cells=(), turn=0)

    with pytest.raises(ValueError, match="Expected a 4x4 board"):
        render_board(malformed)


def test_parse_placement() -> None:
    game = TreSigilli()
    assert parse_move("  5  ", game, game.initial_state()) == Place(5)


def test_parse_rejects_invalid_placement() -> None:
    game = TreSigilli()
    state = game.apply_move(game.initial_state(), Place(5))

    for text in ("", "5 6", "-1", "16", "abc", "５", "5.0", "5"):
        with pytest.raises(ValueError):
            parse_move(text, game, state)


def test_parse_slide() -> None:
    game = TreSigilli()
    state = game.initial_state()

    for cell in (0, 15, 1, 14, 2, 13):
        state = game.apply_move(state, Place(cell))

    assert parse_move("0 4", game, state) == Slide(0, 4)


def test_parse_rejects_illegal_slide() -> None:
    game = TreSigilli()
    state = game.initial_state()

    for cell in (0, 15, 1, 14, 2, 13):
        state = game.apply_move(state, Place(cell))

    for text in ("", "0", "0 5", "15 11", "0 16", "x 4", "0 -4"):
        with pytest.raises(ValueError):
            parse_move(text, game, state)


def test_play_handles_eof_without_traceback() -> None:
    output: list[str] = []

    def read(_: str) -> str:
        raise EOFError

    assert play_human_vs_ai(Player.NORTH, read, output.append) is False
    assert output[-1] == "Game interrupted: end of input."


def test_play_ai_starts_when_human_is_south() -> None:
    output: list[str] = []

    def read(_: str) -> str:
        raise EOFError

    assert play_human_vs_ai(Player.SOUTH, read, output.append) is False
    assert any(line.startswith("AI move: Place(") for line in output)


def test_play_retries_invalid_human_move() -> None:
    output: list[str] = []
    attempts = iter(("invalid", "0"))

    def read(_: str) -> str:
        try:
            return next(attempts)
        except StopIteration as exc:
            raise EOFError from exc

    assert play_human_vs_ai(Player.NORTH, read, output.append) is False
    assert any(line.startswith("Invalid move:") for line in output)
    assert any(line.startswith("AI move:") for line in output)


def test_play_rejects_invalid_player() -> None:
    with pytest.raises(ValueError, match="Invalid human player"):
        play_human_vs_ai(None, lambda _: "0", lambda _: None)  # type: ignore[arg-type]


def test_main_handles_eof(monkeypatch: pytest.MonkeyPatch) -> None:
    from boardlab.cli.tre_sigilli import main

    def eof(_: str) -> str:
        raise EOFError

    monkeypatch.setattr("builtins.input", eof)
    assert main() == 1


def test_main_retries_invalid_side(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    from boardlab.cli.tre_sigilli import main

    answers = iter(("invalid", "N"))

    def read(_: str) -> str:
        try:
            return next(answers)
        except StopIteration as exc:
            raise EOFError from exc

    monkeypatch.setattr("builtins.input", read)

    assert main() == 1
    assert "Invalid side" in capsys.readouterr().out
