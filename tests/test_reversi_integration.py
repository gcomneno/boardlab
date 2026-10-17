"""End-to-end Reversi integration with generic match coordination."""

from random import Random

import pytest

from boardlab.engine.game import Game, Outcome
from boardlab.engine.match import Match
from boardlab.engine.strategy import Strategy
from boardlab.games.reversi import Move, Pass, Place, Player, Reversi, State
from boardlab.strategies.random import RandomStrategy


def make_match() -> Match[State, Move, Player]:
    game: Game[State, Move, Player] = Reversi()
    return Match(game)


def play_seeded(seed: int) -> tuple[tuple[State, ...], int]:
    game = Reversi()
    match = make_match()
    strategy: Strategy[State, Move, Player] = RandomStrategy(Random(seed))
    history = [match.state]
    passes = 0

    for _ in range(100):
        before = match.state

        if game.is_terminal(before):
            break

        legal = game.legal_moves(before)
        assert legal

        if legal == (Pass(),):
            passes += 1

        snapshot = before.cells
        successor = match.play_strategy(strategy)

        assert successor is match.state
        assert successor is not before
        assert successor.active_player is not before.active_player
        assert before.cells == snapshot
        assert history[-1] is before

        history.append(successor)
    else:
        pytest.fail("Reversi match exceeded 100 actions")

    terminal = match.state
    assert game.is_terminal(terminal)
    assert game.legal_moves(terminal) == ()

    outcomes = (
        game.outcome(terminal, Player.BLACK),
        game.outcome(terminal, Player.WHITE),
    )
    assert outcomes in {
        (Outcome.WIN, Outcome.LOSS),
        (Outcome.LOSS, Outcome.WIN),
        (Outcome.DRAW, Outcome.DRAW),
    }

    with pytest.raises(ValueError, match="terminal"):
        match.play_strategy(strategy)

    assert match.state is terminal
    return tuple(history), passes


@pytest.mark.parametrize("seed", [0, 1, 7, 42, 2026])
def test_seeded_random_match_reaches_terminal_outcome(seed: int) -> None:
    history, passes = play_seeded(seed)

    assert len(history) > 1
    assert passes >= 0


@pytest.mark.parametrize("seed", [1, 7, 2026])
def test_seeded_match_exercises_explicit_pass(seed: int) -> None:
    history, passes = play_seeded(seed)

    assert passes > 0
    assert any(
        before.cells == after.cells and before.active_player is not after.active_player
        for before, after in zip(history, history[1:], strict=False)
    )


def test_seeded_random_match_is_reproducible() -> None:
    assert play_seeded(42) == play_seeded(42)


def test_illegal_move_preserves_match_state() -> None:
    match = make_match()
    initial = match.state

    with pytest.raises(ValueError, match="Illegal move"):
        match.play(Place(0))

    assert match.state is initial

    successor = match.play(Place(8))
    assert successor is match.state

    with pytest.raises(ValueError, match="Illegal move"):
        match.play(Place(8))

    assert match.state is successor


def test_strategy_failure_preserves_match_state() -> None:
    class FailingStrategy:
        def choose_move(
            self,
            game: Game[State, Move, Player],
            state: State,
        ) -> Move:
            raise RuntimeError("Deliberate strategy failure")

    match = make_match()
    initial = match.state

    with pytest.raises(RuntimeError, match="Deliberate strategy failure"):
        match.play_strategy(FailingStrategy())

    assert match.state is initial
