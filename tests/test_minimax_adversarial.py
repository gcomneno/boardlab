"""Adversarial regression tests for depth-limited Minimax."""

from dataclasses import dataclass
from math import inf

import pytest

from boardlab.engine.game import Game, Outcome
from boardlab.strategies.minimax import MinimaxStrategy


@dataclass(frozen=True)
class TreeState:
    name: str


class TreeGame:
    """Small deterministic two-player tree with explicit outcomes."""

    def __init__(
        self,
        children: dict[str, tuple[str, ...]],
        outcomes: dict[str, Outcome] | None = None,
    ) -> None:
        self.children = children
        self.outcomes = outcomes or {}

    def initial_state(self) -> TreeState:
        return TreeState("root")

    def current_player(self, state: TreeState) -> str:
        if self.is_terminal(state):
            raise ValueError("No active player")
        return "MAX" if state.name == "root" else "MIN"

    def legal_moves(self, state: TreeState) -> tuple[str, ...]:
        return self.children.get(state.name, ())

    def apply_move(self, state: TreeState, move: str) -> TreeState:
        if move not in self.legal_moves(state):
            raise ValueError("Illegal move")
        return TreeState(move)

    def is_terminal(self, state: TreeState) -> bool:
        return state.name in self.outcomes

    def outcome(self, state: TreeState, player: str) -> Outcome:
        if not self.is_terminal(state):
            raise ValueError("Not terminal")
        result = self.outcomes[state.name]
        if player == "MAX":
            return result
        if result is Outcome.WIN:
            return Outcome.LOSS
        if result is Outcome.LOSS:
            return Outcome.WIN
        return Outcome.DRAW


def evaluate(
    game: Game[TreeState, str, str],
    state: TreeState,
    player: str,
) -> float:
    scores = {
        "trap_good": 100.0,
        "trap_bad": -100.0,
        "safe_a": 3.0,
        "safe_b": 2.0,
    }
    return scores[state.name]


def test_opponent_minimizes_root_score() -> None:
    game = TreeGame(
        {
            "root": ("trap", "safe"),
            "trap": ("trap_good", "trap_bad"),
            "safe": ("safe_a", "safe_b"),
        }
    )
    strategy = MinimaxStrategy[TreeState, str, str](evaluate, depth=2)

    assert strategy.choose_move(game, game.initial_state()) == "safe"
    assert strategy.last_stats.nodes_visited == 7
    assert strategy.last_stats.evaluated_leaves == 4


def test_forced_win_beats_finite_heuristic() -> None:
    game = TreeGame(
        {"root": ("heuristic", "win")},
        {"win": Outcome.WIN},
    )

    def optimistic(
        game: Game[TreeState, str, str],
        state: TreeState,
        player: str,
    ) -> float:
        return 1_000_000.0

    strategy = MinimaxStrategy[TreeState, str, str](optimistic, depth=1)

    assert strategy.choose_move(game, game.initial_state()) == "win"


def test_all_forced_losses_choose_first_legal_move() -> None:
    game = TreeGame(
        {"root": ("loss_a", "loss_b")},
        {
            "loss_a": Outcome.LOSS,
            "loss_b": Outcome.LOSS,
        },
    )
    strategy = MinimaxStrategy[TreeState, str, str](evaluate, depth=1)

    assert strategy.choose_move(game, game.initial_state()) == "loss_a"
    assert strategy.last_stats.nodes_visited == 3
    assert strategy.last_stats.evaluated_leaves == 2


def test_failed_search_does_not_publish_partial_metrics() -> None:
    game = TreeGame({"root": ("trap_good", "trap_bad")})
    calls = 0

    def sometimes_fails(
        game: Game[TreeState, str, str],
        state: TreeState,
        player: str,
    ) -> float:
        nonlocal calls
        calls += 1
        if calls == 2:
            return inf
        return 1.0

    strategy = MinimaxStrategy[TreeState, str, str](sometimes_fails, depth=1)
    original = strategy.last_stats

    with pytest.raises(ValueError, match="finite"):
        strategy.choose_move(game, game.initial_state())

    assert strategy.last_stats == original
