"""Dado Goal: deterministic reconstruction of author-confirmed rules.

Fair and independent dice rolls are optional digital modelling assumptions.
This module does not generate random results or implement strategic AI.
"""

from dataclasses import dataclass
from enum import Enum


class Phase(Enum):
    TEAM = "team"
    PINK = "pink"
    RESULT = "result"
    FINISHED = "finished"


class TeamFace(Enum):
    SHOT = "Tiro in porta!"
    INTERCEPTED = "Passaggio intercettato!"
    CROSS = "Cross!"
    PASS = "Passaggio"


class PinkFace(Enum):
    BOTTOM = "Palla sul fondo"
    HALF_BICYCLE = "Mezza rovesciata"
    ABOVE_BAR = "Palla alta sulla traversa"
    INTERCEPTED = "Palla intercettata"
    BICYCLE = "Rovesciata"
    HEADER = "Colpo di testa"


class ResultFace(Enum):
    REBOUND = "Respinto!"
    GOAL = "GOAL!"
    WIDE = "FUORI!"
    WOODWORK = "PALO/TRAVERSA"
    PENALTY = "RIGORE"
    SAVED = "Parato!"


TEAM_FACES = (
    TeamFace.SHOT,
    TeamFace.SHOT,
    TeamFace.INTERCEPTED,
    TeamFace.INTERCEPTED,
    TeamFace.CROSS,
    TeamFace.PASS,
)

PINK_FACES = tuple(PinkFace)
RESULT_FACES = tuple(ResultFace)

type DieFace = TeamFace | PinkFace | ResultFace


@dataclass(frozen=True)
class State:
    """Complete match state; players are identified by indices 0 and 1."""

    hands_per_team: int
    active_player: int = 0
    completed_hands: tuple[int, int] = (0, 0)
    goals: tuple[int, int] = (0, 0)
    phase: Phase = Phase.TEAM


def initial_state(hands_per_team: int) -> State:
    """Create a match with a positive number of hands per team."""
    if type(hands_per_team) is not int or hands_per_team < 1:
        raise ValueError("hands_per_team must be a positive integer")
    return State(hands_per_team=hands_per_team)


def _validate(state: State) -> None:
    """Reject inconsistent or malformed match states."""
    if not isinstance(state, State):
        raise ValueError("Invalid Dado Goal state")

    n = state.hands_per_team
    hands = state.completed_hands
    goals = state.goals

    valid = (
        type(n) is int
        and n > 0
        and type(state.active_player) is int
        and state.active_player in (0, 1)
        and isinstance(state.phase, Phase)
        and type(hands) is tuple
        and len(hands) == 2
        and all(type(x) is int and 0 <= x <= n for x in hands)
        and type(goals) is tuple
        and len(goals) == 2
        and all(type(x) is int and 0 <= x <= hands[i] for i, x in enumerate(goals))
    )

    if not valid:
        raise ValueError("Invalid Dado Goal state")

    total = sum(hands)
    expected_hands = ((total + 1) // 2, total // 2)

    if hands != expected_hands:
        raise ValueError("Invalid Dado Goal state")

    finished = hands == (n, n)

    if (state.phase is Phase.FINISHED) != finished:
        raise ValueError("Invalid Dado Goal state")

    if not finished and state.active_player != total % 2:
        raise ValueError("Invalid Dado Goal state")


def _end_hand(state: State, scored: bool = False) -> State:
    player = state.active_player

    hands = list(state.completed_hands)
    goals = list(state.goals)

    hands[player] += 1
    if scored:
        goals[player] += 1

    finished = tuple(hands) == (state.hands_per_team,) * 2

    return State(
        hands_per_team=state.hands_per_team,
        active_player=1 - player,
        completed_hands=(hands[0], hands[1]),
        goals=(goals[0], goals[1]),
        phase=Phase.FINISHED if finished else Phase.TEAM,
    )


def apply_roll(state: State, face: DieFace) -> State:
    """Apply one actual die result without mutating the input state."""
    _validate(state)

    if state.phase is Phase.FINISHED:
        raise ValueError("Cannot roll after the match has finished")

    if state.phase is Phase.TEAM:
        if not isinstance(face, TeamFace):
            raise ValueError("Expected a team-die face")
        if face is TeamFace.PASS:
            return state
        if face is TeamFace.INTERCEPTED:
            return _end_hand(state)
        return State(
            state.hands_per_team,
            state.active_player,
            state.completed_hands,
            state.goals,
            Phase.PINK,
        )

    if state.phase is Phase.PINK:
        if not isinstance(face, PinkFace):
            raise ValueError("Expected a pink-die face")
        if face in (PinkFace.HALF_BICYCLE, PinkFace.BICYCLE, PinkFace.HEADER):
            return State(
                state.hands_per_team,
                state.active_player,
                state.completed_hands,
                state.goals,
                Phase.RESULT,
            )
        return _end_hand(state)

    if not isinstance(face, ResultFace):
        raise ValueError("Expected a result-die face")
    if face is ResultFace.PENALTY:
        return state
    return _end_hand(state, scored=face is ResultFace.GOAL)
