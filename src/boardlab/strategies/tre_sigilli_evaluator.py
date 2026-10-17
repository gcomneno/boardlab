"""Territorial heuristic for Tre Sigilli."""

from boardlab.engine.game import Game
from boardlab.games.tre_sigilli import Move, Player, State, TreSigilli


def evaluate_territory(
    game: Game[State, Move, Player],
    state: State,
    player: Player,
) -> float:
    """Return territorial advantage from the requested player's perspective."""
    if not isinstance(game, TreSigilli):
        raise TypeError("Territorial evaluator requires TreSigilli")

    north, south = game.territory_scores(state)
    difference = north - south

    return float(difference if player is Player.NORTH else -difference)
