"""Deterministic invariants for Monte Carlo Tree Search."""

from dataclasses import dataclass
from random import Random

import pytest

from boardlab.engine.game import Outcome
from boardlab.strategies.mcts import MCTSStrategy


@dataclass(frozen=True)
class State:
    step: int = 0


class CountingGame:
    """One-move game exposing the exact number of simulations."""

    def __init__(self) -> None:
        self.outcome_calls = 0
        self.apply_calls = 0

    def initial_state(self) -> State:
        return State()

    def current_player(self, state: State) -> str:
        if self.is_terminal(state):
            raise ValueError("Terminal state")
        return "first"

    def legal_moves(self, state: State) -> tuple[str, ...]:
        return ("only",) if state.step == 0 else ()

    def apply_move(self, state: State, move: str) -> State:
        if move not in self.legal_moves(state):
            raise ValueError("Illegal move")
        self.apply_calls += 1
        return State(1)

    def is_terminal(self, state: State) -> bool:
        return state.step == 1

    def outcome(self, state: State, player: str) -> Outcome:
        if not self.is_terminal(state):
            raise ValueError("Not terminal")
        self.outcome_calls += 1
        return Outcome.DRAW


@pytest.mark.parametrize("budget", [1, 2, 10, 100])
def test_exact_simulation_budget(budget: int) -> None:
    game = CountingGame()
    strategy = MCTSStrategy[State, str, str](budget, rng=Random(42))

    assert strategy.choose_move(game, game.initial_state()) == "only"
    assert game.outcome_calls == budget
    assert game.apply_calls == 1


def test_state_is_not_modified() -> None:
    game = CountingGame()
    state = game.initial_state()
    strategy = MCTSStrategy[State, str, str](20, rng=Random(42))

    strategy.choose_move(game, state)

    assert state == State(0)


class TieGame(CountingGame):
    """Two equivalent actions, both producing terminal draws."""

    def legal_moves(self, state: State) -> tuple[str, ...]:
        return ("first", "second") if state.step == 0 else ()


def test_root_tie_uses_legal_move_order() -> None:
    game = TieGame()
    strategy = MCTSStrategy[State, str, str](2, rng=Random(42))

    assert strategy.choose_move(game, game.initial_state()) == "first"


def test_repeated_seed_produces_same_choice() -> None:
    game = TieGame()
    state = game.initial_state()

    first = MCTSStrategy[State, str, str](25, rng=Random(1234))
    second = MCTSStrategy[State, str, str](25, rng=Random(1234))

    assert first.choose_move(game, state) == second.choose_move(game, state)
