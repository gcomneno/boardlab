"""Monte Carlo Tree Search with UCT and reproducible random rollouts."""

from dataclasses import dataclass, field
from math import isfinite, log, sqrt
from random import Random

from boardlab.engine.game import Game, Outcome


@dataclass
class _Node[StateT, MoveT, PlayerT]:
    state: StateT
    player_just_moved: PlayerT
    move: MoveT | None = None
    visits: int = 0
    reward: float = 0.0
    children: list["_Node[StateT, MoveT, PlayerT]"] = field(default_factory=list)
    untried_moves: list[MoveT] = field(default_factory=list)


class MCTSStrategy[StateT, MoveT, PlayerT]:
    """Select moves by UCT search using a fixed simulation budget."""

    def __init__(
        self,
        simulations: int,
        *,
        exploration: float = sqrt(2.0),
        rng: Random | None = None,
    ) -> None:
        if type(simulations) is not int or simulations < 1:
            raise ValueError("Simulations must be a positive integer")
        if not isfinite(exploration) or exploration < 0:
            raise ValueError("Exploration must be finite and nonnegative")

        self._simulations = simulations
        self._exploration = exploration
        self._rng = rng if rng is not None else Random()

    def choose_move(
        self,
        game: Game[StateT, MoveT, PlayerT],
        state: StateT,
    ) -> MoveT:
        """Choose the most visited root move without modifying the input state."""
        moves = game.legal_moves(state)
        if not moves:
            raise ValueError("No legal moves")

        root_player = game.current_player(state)
        root = _Node[StateT, MoveT, PlayerT](
            state=state,
            player_just_moved=root_player,
            untried_moves=list(moves),
        )

        for _ in range(self._simulations):
            node = root
            path = [node]

            # Selection: descend while the current node is fully expanded.
            while not game.is_terminal(node.state) and not node.untried_moves and node.children:
                node = self._select_child(node)
                path.append(node)

            # Expansion: choose one previously unexpanded legal move.
            if not game.is_terminal(node.state) and node.untried_moves:
                index = self._rng.randrange(len(node.untried_moves))
                move = node.untried_moves.pop(index)
                player = game.current_player(node.state)
                successor = game.apply_move(node.state, move)
                child = _Node[StateT, MoveT, PlayerT](
                    state=successor,
                    player_just_moved=player,
                    move=move,
                    untried_moves=list(game.legal_moves(successor)),
                )
                node.children.append(child)
                node = child
                path.append(node)

            # Simulation: complete a random legal game.
            rollout = node.state
            while not game.is_terminal(rollout):
                rollout_moves = game.legal_moves(rollout)
                rollout = game.apply_move(
                    rollout,
                    self._rng.choice(rollout_moves),
                )

            # Backpropagation: each node uses its own player's perspective.
            for visited in path[1:]:
                outcome = game.outcome(rollout, visited.player_just_moved)
                visited.visits += 1
                visited.reward += self._reward(outcome)

            root.visits += 1

        # Stable tie-breaking follows the original legal-move ordering.
        return max(
            moves,
            key=lambda move: next(
                (child.visits for child in root.children if child.move == move),
                0,
            ),
        )

    def _select_child(
        self,
        parent: _Node[StateT, MoveT, PlayerT],
    ) -> _Node[StateT, MoveT, PlayerT]:
        """Apply UCT from the perspective of the player making this move."""
        return max(
            parent.children,
            key=lambda child: (
                child.reward / child.visits
                + self._exploration * sqrt(log(parent.visits) / child.visits)
            ),
        )

    @staticmethod
    def _reward(outcome: Outcome) -> float:
        if outcome is Outcome.WIN:
            return 1.0
        if outcome is Outcome.DRAW:
            return 0.5
        return 0.0
