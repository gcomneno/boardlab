"""Behavioral tests for generic match coordination."""

import pytest

from boardlab.engine.game import Game, Outcome
from boardlab.engine.match import Match
from boardlab.games.three_stones import Player, State, ThreeStones


def make_match() -> Match[State, int, Player]:
    game: Game[State, int, Player] = ThreeStones()
    return Match(game)


def test_match_starts_from_game_initial_state() -> None:
    match = make_match()

    assert match.state == State((None, None, None), Player.NORTH)


def test_match_advances_and_returns_successor() -> None:
    match = make_match()
    initial = match.state

    successor = match.play(0)

    assert successor is match.state
    assert successor == State((Player.NORTH, None, None), Player.SOUTH)
    assert initial == State((None, None, None), Player.NORTH)

    second = match.play(1)

    assert second == State((Player.NORTH, Player.SOUTH, None), Player.NORTH)
    assert successor == State((Player.NORTH, None, None), Player.SOUTH)


def test_illegal_move_does_not_advance_match() -> None:
    match = make_match()
    first = match.play(0)

    with pytest.raises(ValueError, match="Illegal move"):
        match.play(0)

    assert match.state is first


def test_match_can_reach_terminal_state() -> None:
    game = ThreeStones()
    match = make_match()

    for move in (0, 1, 2):
        match.play(move)

    assert game.is_terminal(match.state)
    assert game.outcome(match.state, Player.NORTH) is Outcome.WIN
    assert game.outcome(match.state, Player.SOUTH) is Outcome.LOSS

    terminal = match.state

    with pytest.raises(ValueError, match="Illegal move"):
        match.play(0)

    assert match.state is terminal


def test_independent_matches_do_not_share_progress() -> None:
    first = make_match()
    second = make_match()

    first.play(0)

    assert first.state == State((Player.NORTH, None, None), Player.SOUTH)
    assert second.state == State((None, None, None), Player.NORTH)


def test_play_strategy_advances_match() -> None:
    from boardlab.engine.game import Game
    from boardlab.games.three_stones import Player, State

    class FirstLegalMove:
        def choose_move(
            self,
            game: Game[State, int, Player],
            state: State,
        ) -> int:
            return game.legal_moves(state)[0]

    game = ThreeStones()
    match = Match(game)
    original = match.state

    successor = match.play_strategy(FirstLegalMove())

    assert successor == game.apply_move(original, 0)
    assert match.state is successor


def test_play_strategy_rejects_illegal_move_without_advancing() -> None:
    from boardlab.engine.game import Game
    from boardlab.games.three_stones import Player, State

    class IllegalStrategy:
        def choose_move(
            self,
            game: Game[State, int, Player],
            state: State,
        ) -> int:
            return 99

    match = Match(ThreeStones())
    original = match.state

    with pytest.raises(ValueError):
        match.play_strategy(IllegalStrategy())

    assert match.state is original


def test_play_strategy_preserves_state_on_strategy_failure() -> None:
    from boardlab.engine.game import Game
    from boardlab.games.three_stones import Player, State

    class FailingStrategy:
        def choose_move(
            self,
            game: Game[State, int, Player],
            state: State,
        ) -> int:
            raise RuntimeError("Strategy failure")

    match = Match(ThreeStones())
    original = match.state

    with pytest.raises(RuntimeError, match="Strategy failure"):
        match.play_strategy(FailingStrategy())

    assert match.state is original


def test_play_strategy_rejects_terminal_state_before_selection() -> None:
    from boardlab.engine.game import Game
    from boardlab.games.three_stones import Player, State

    class NeverCalledStrategy:
        def choose_move(
            self,
            game: Game[State, int, Player],
            state: State,
        ) -> int:
            raise AssertionError("Strategy must not be called")

    match = Match(ThreeStones())

    for move in (0, 1, 2):
        match.play(move)

    terminal = match.state

    with pytest.raises(ValueError, match="terminal"):
        match.play_strategy(NeverCalledStrategy())

    assert match.state is terminal
