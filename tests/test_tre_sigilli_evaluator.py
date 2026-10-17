"""Contract tests for the Tre Sigilli territorial evaluator."""

from boardlab.games.tre_sigilli import Move, Place, Player, State, TreSigilli
from boardlab.strategies.minimax import MinimaxStrategy
from boardlab.strategies.tre_sigilli_evaluator import evaluate_territory


def placements(*cells: int) -> State:
    game = TreSigilli()
    state = game.initial_state()
    for cell in cells:
        state = game.apply_move(state, Place(cell))
    return state


def test_empty_board_is_neutral() -> None:
    game = TreSigilli()
    state = game.initial_state()

    assert evaluate_territory(game, state, Player.NORTH) == 0.0
    assert evaluate_territory(game, state, Player.SOUTH) == 0.0


def test_single_north_seal_gives_positive_advantage() -> None:
    game = TreSigilli()
    state = placements(0)

    assert evaluate_territory(game, state, Player.NORTH) == 3.0
    assert evaluate_territory(game, state, Player.SOUTH) == -3.0


def test_contested_territory_uses_score_difference() -> None:
    game = TreSigilli()
    state = placements(0, 2)

    assert game._territory_scores(state) == (2, 3)
    assert evaluate_territory(game, state, Player.NORTH) == -1.0
    assert evaluate_territory(game, state, Player.SOUTH) == 1.0


def test_player_perspectives_are_opposites() -> None:
    game = TreSigilli()

    for state in (
        game.initial_state(),
        placements(0),
        placements(0, 2),
        placements(0, 2, 5, 7),
    ):
        north = evaluate_territory(game, state, Player.NORTH)
        south = evaluate_territory(game, state, Player.SOUTH)

        assert north == -south


def test_evaluation_does_not_mutate_state() -> None:
    game = TreSigilli()
    state = placements(0, 2, 5, 7)
    original = state

    evaluate_territory(game, state, Player.NORTH)

    assert state == original


def test_minimax_accepts_territorial_evaluator() -> None:
    game = TreSigilli()
    state = game.initial_state()
    strategy = MinimaxStrategy[State, Move, Player](
        evaluate_territory,
        depth=1,
    )

    move = strategy.choose_move(game, state)

    assert move in game.legal_moves(state)
    assert strategy.last_stats.nodes_visited == 17
    assert strategy.last_stats.evaluated_leaves == 16
