"""Adversarial checks for generic Monte Carlo Tree Search."""

from dataclasses import dataclass
from math import log, sqrt
from random import Random

import pytest

from boardlab.engine.game import Outcome
from boardlab.strategies.mcts import MCTSStrategy, _Node


@dataclass(eq=True)
class UnhashableMove:
    name: str


@dataclass(frozen=True)
class TreeState:
    name: str


class TreeGame:
    """A small deterministic game with an opponent-controlled trap."""

    def initial_state(self) -> TreeState:
        return TreeState("root")

    def current_player(self, state: TreeState) -> str:
        if state.name == "root":
            return "north"
        if state.name in ("safe", "trap"):
            return "south"
        raise ValueError("Terminal state")

    def legal_moves(self, state: TreeState) -> tuple[UnhashableMove, ...]:
        edges = {
            "root": ("safe", "trap"),
            "safe": ("safe_a", "safe_b"),
            "trap": ("trap_bad", "trap_good"),
        }
        return tuple(UnhashableMove(name) for name in edges.get(state.name, ()))

    def apply_move(self, state: TreeState, move: UnhashableMove) -> TreeState:
        if move not in self.legal_moves(state):
            raise ValueError("Illegal move")
        return TreeState(move.name)

    def is_terminal(self, state: TreeState) -> bool:
        return not self.legal_moves(state)

    def outcome(self, state: TreeState, player: str) -> Outcome:
        if not self.is_terminal(state):
            raise ValueError("Not terminal")
        north_wins = state.name in ("safe_a", "safe_b", "trap_good")
        winner = "north" if north_wins else "south"
        return Outcome.WIN if player == winner else Outcome.LOSS


def test_unhashable_moves_and_adversarial_opponent() -> None:
    game = TreeGame()
    strategy: MCTSStrategy[TreeState, UnhashableMove, str] = MCTSStrategy(600, rng=Random(42))

    assert strategy.choose_move(game, game.initial_state()) == UnhashableMove("safe")


def test_uct_prefers_exploitation_when_exploration_is_zero() -> None:
    strategy = MCTSStrategy[TreeState, str, str](10, exploration=0.0, rng=Random(1))
    parent = _Node[TreeState, str, str](TreeState("root"), "south", visits=20)
    strong = _Node[TreeState, str, str](TreeState("a"), "north", visits=10, reward=9.0)
    weak = _Node[TreeState, str, str](TreeState("b"), "north", visits=10, reward=2.0)
    parent.children = [weak, strong]

    assert strategy._select_child(parent) is strong


def test_uct_exploration_bonus() -> None:
    strategy = MCTSStrategy[TreeState, str, str](10, exploration=sqrt(2.0), rng=Random(1))
    parent = _Node[TreeState, str, str](TreeState("root"), "south", visits=100)
    frequent = _Node[TreeState, str, str](TreeState("a"), "north", visits=90, reward=54.0)
    rare = _Node[TreeState, str, str](TreeState("b"), "north", visits=10, reward=5.0)
    parent.children = [frequent, rare]

    assert 0.6 + sqrt(2 * log(100) / 90) < (0.5 + sqrt(2 * log(100) / 10))
    assert strategy._select_child(parent) is rare


@pytest.mark.parametrize(
    ("outcome", "expected"),
    [
        (Outcome.WIN, 1.0),
        (Outcome.DRAW, 0.5),
        (Outcome.LOSS, 0.0),
    ],
)
def test_terminal_reward(outcome: Outcome, expected: float) -> None:
    assert MCTSStrategy._reward(outcome) == expected
