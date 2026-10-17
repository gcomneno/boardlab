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
