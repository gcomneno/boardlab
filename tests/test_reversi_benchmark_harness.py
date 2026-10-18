"""Focused tests for the Reversi benchmark harness."""

from dataclasses import FrozenInstanceError
from typing import cast

import pytest
from benchmarks import reversi_6x6 as benchmark

from boardlab.engine.game import Outcome
from boardlab.games.reversi import Player


def test_configuration_defaults_and_validation() -> None:
    config = benchmark.Configuration()
    assert config.depth == 2
    assert config.simulations == 32

    for invalid in (0, -1, True, 1.5):
        with pytest.raises(ValueError):
            benchmark.Configuration(depth=cast(int, invalid))
        with pytest.raises(ValueError):
            benchmark.Configuration(simulations=cast(int, invalid))


def test_configuration_is_immutable() -> None:
    with pytest.raises(FrozenInstanceError):
        benchmark.Configuration().depth = 3  # type: ignore[misc]


def test_invalid_match_arguments() -> None:
    config = benchmark.Configuration()

    with pytest.raises(ValueError):
        benchmark.play_match("random", 0, Player.BLACK, config)

    with pytest.raises(ValueError):
        benchmark.play_match("minimax", -1, Player.BLACK, config)

    with pytest.raises(ValueError):
        benchmark.play_match("minimax", True, Player.BLACK, config)

    with pytest.raises(ValueError):
        benchmark.play_match("minimax", 0, cast(Player, "BLACK"), config)


@pytest.mark.parametrize(
    ("candidate", "player"),
    [
        ("minimax", Player.BLACK),
        ("alpha_beta", Player.WHITE),
        ("mcts", Player.BLACK),
    ],
)
def test_match_result_contract(
    candidate: benchmark.StrategyName,
    player: Player,
) -> None:
    config = benchmark.Configuration(depth=1, simulations=4)
    result = benchmark.play_match(candidate, 0, player, config)

    assert result.candidate == candidate
    assert result.seed == 0
    assert result.candidate_player is player
    assert result.outcome in (Outcome.WIN, Outcome.LOSS, Outcome.DRAW)
    assert result.actions > 0
    assert result.elapsed_seconds >= 0

    if candidate == "mcts":
        assert result.nodes_visited is None
        assert result.evaluated_leaves is None
    else:
        assert result.nodes_visited is not None
        assert result.evaluated_leaves is not None
        assert result.nodes_visited > 0
        assert result.evaluated_leaves > 0


def test_reproducibility_excludes_wall_clock() -> None:
    config = benchmark.Configuration(depth=1, simulations=4)
    first = benchmark.play_match("mcts", 1, Player.WHITE, config)
    second = benchmark.play_match("mcts", 1, Player.WHITE, config)

    assert (
        first.candidate,
        first.seed,
        first.candidate_player,
        first.outcome,
        first.actions,
        first.nodes_visited,
        first.evaluated_leaves,
    ) == (
        second.candidate,
        second.seed,
        second.candidate_player,
        second.outcome,
        second.actions,
        second.nodes_visited,
        second.evaluated_leaves,
    )


def test_both_candidate_roles() -> None:
    config = benchmark.Configuration(depth=1, simulations=4)

    black = benchmark.play_match("minimax", 2, Player.BLACK, config)
    white = benchmark.play_match("minimax", 2, Player.WHITE, config)

    assert black.candidate_player is Player.BLACK
    assert white.candidate_player is Player.WHITE
    assert black.seed == white.seed == 2


def test_pilot_schedule_without_executing_matches(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    calls: list[tuple[benchmark.StrategyName, int, Player]] = []

    def record(
        candidate: benchmark.StrategyName,
        seed: int,
        player: Player,
        config: benchmark.Configuration,
    ) -> object:
        calls.append((candidate, seed, player))
        return object()

    monkeypatch.setattr(benchmark, "play_match", record)

    config = benchmark.Configuration()
    results = benchmark.pilot(config, seeds=(0, 1))

    assert len(results) == 12
    assert calls == [
        (candidate, seed, player)
        for candidate in ("minimax", "alpha_beta", "mcts")
        for seed in (0, 1)
        for player in (Player.BLACK, Player.WHITE)
    ]
