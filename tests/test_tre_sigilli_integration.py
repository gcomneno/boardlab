"""End-to-end integration of Tre Sigilli with generic match coordination."""

from random import Random

import pytest

from boardlab.engine.game import Game, Outcome
from boardlab.engine.match import Match
from boardlab.engine.strategy import Strategy
from boardlab.games.tre_sigilli import Place, Player, Slide, State, TreSigilli
from boardlab.strategies.random import RandomStrategy

type Move = Place | Slide


def make_match() -> Match[State, Move, Player]:
    """Exercise Tre Sigilli through the generic Game contract."""
    game: Game[State, Move, Player] = TreSigilli()
    return Match(game)


@pytest.mark.parametrize("seed", [0, 1, 7, 42, 2026])
def test_seeded_random_match_reaches_valid_terminal_outcome(seed: int) -> None:
    game = TreSigilli()
    match = make_match()
    strategy: Strategy[State, Move, Player] = RandomStrategy(Random(seed))

    initial = match.state
    history = [initial]

    for _ in range(18):
        current = match.state
        assert current is history[-1]

        if game.is_terminal(current):
            break

        active = game.current_player(current)
        assert active is (Player.NORTH if current.turn % 2 == 0 else Player.SOUTH)

        legal = game.legal_moves(current)
        assert legal

        successor = match.play_strategy(strategy)

        assert successor is match.state
        assert successor is not current
        assert successor.turn == current.turn + 1
        assert current == history[-1]

        if current.turn < 6:
            assert sum(cell is not None for cell in successor.cells) == current.turn + 1
        else:
            assert sum(cell is not None for cell in successor.cells) == 6

        history.append(successor)
    else:
        assert match.state.turn == 18

    terminal = match.state
    assert game.is_terminal(terminal)
    assert 6 <= terminal.turn <= 18
    assert len(history) == terminal.turn + 1
    assert initial == game.initial_state()

    north = game.outcome(terminal, Player.NORTH)
    south = game.outcome(terminal, Player.SOUTH)

    assert (north, south) in {
        (Outcome.WIN, Outcome.LOSS),
        (Outcome.LOSS, Outcome.WIN),
        (Outcome.DRAW, Outcome.DRAW),
    }

    with pytest.raises(ValueError, match="terminal"):
        match.play_strategy(strategy)

    assert match.state is terminal


def test_illegal_move_preserves_match_state() -> None:
    match = make_match()
    initial = match.state

    with pytest.raises(ValueError, match="Illegal move"):
        match.play(Place(16))

    assert match.state is initial

    successor = match.play(Place(0))
    assert successor is match.state

    with pytest.raises(ValueError, match="Illegal move"):
        match.play(Place(0))

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


def test_seeded_random_strategy_is_reproducible() -> None:
    def play(seed: int) -> tuple[State, ...]:
        game = TreSigilli()
        match = make_match()
        strategy: Strategy[State, Move, Player] = RandomStrategy(Random(seed))
        history = [match.state]

        while not game.is_terminal(match.state):
            history.append(match.play_strategy(strategy))

        return tuple(history)

    assert play(42) == play(42)
