"""Small deterministic CSP placement puzzles for the Ark.

The generic solver supports equality and inequality, but not ordering.
Clues therefore describe positions and exclusions, with one animal per stall.
"""

from dataclasses import dataclass

from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    ConstraintKind,
    Problem,
    Variable,
)
from boardlab.puzzles.generic.solver import satisfies, solve

ANIMALS = ("zebra", "coniglio", "tartaruga", "elefante")
STALLS = (0, 1, 2, 3)

_LABELS_EN = {
    "zebra": "The zebra",
    "coniglio": "The rabbit",
    "tartaruga": "The turtle",
    "elefante": "The elephant",
}

_LABELS = {
    "zebra": "La zebra",
    "coniglio": "Il coniglio",
    "tartaruga": "La tartaruga",
    "elefante": "L'elefante",
}


@dataclass(frozen=True)
class ArkLevel:
    difficulty: str
    problem: Problem
    solution: tuple[int, ...]
    clues: tuple[str, ...]


def _candidate_constraints(
    solution: tuple[int, ...], difficulty: str, locale: str = "it"
) -> list[tuple[Constraint, str]]:
    """Build truthful clue candidates, hardest configurations first."""
    candidates = []
    labels = _LABELS if locale == "it" else _LABELS_EN
    for i, animal in enumerate(ANIMALS):
        position = solution[i]
        for stall in STALLS:
            anchor = f"stall_{stall}"
            if stall == position:
                kind = ConstraintKind.EQUAL
                text = (
                    f"{labels[animal]} occupa il compartimento {stall + 1}."
                    if locale == "it"
                    else f"{labels[animal]} occupies compartment {stall + 1}."
                )
            else:
                kind = ConstraintKind.NOT_EQUAL
                text = (
                    f"{labels[animal]} non può stare nel compartimento {stall + 1}."
                    if locale == "it"
                    else f"{labels[animal]} cannot be in compartment {stall + 1}."
                )
            candidates.append(
                (
                    Constraint(kind, (animal, anchor)),
                    text,
                )
            )

    if difficulty == "facile":
        return [c for c in candidates if c[0].kind is ConstraintKind.EQUAL] + [
            c for c in candidates if c[0].kind is ConstraintKind.NOT_EQUAL
        ]
    if difficulty == "medio":
        return [c for c in candidates if c[0].kind is ConstraintKind.NOT_EQUAL] + [
            c for c in candidates if c[0].kind is ConstraintKind.EQUAL
        ]
    return [c for c in candidates if c[0].kind is ConstraintKind.NOT_EQUAL]


def build_level(difficulty: str = "facile", locale: str = "it") -> ArkLevel:
    """Choose a minimal clue set with a certified unique solution."""
    if difficulty not in ("facile", "medio", "difficile"):
        raise ValueError("Unknown difficulty")

    if locale not in ("it", "en"):
        raise ValueError("Unsupported locale")

    target = {
        "facile": (0, 1, 2, 3),
        "medio": (1, 3, 0, 2),
        "difficile": (2, 0, 3, 1),
    }[difficulty]

    variables = tuple(Variable(a, STALLS) for a in ANIMALS) + tuple(
        Variable(f"stall_{i}", (i,)) for i in STALLS
    )
    uniqueness = Constraint(ConstraintKind.ALL_DIFFERENT, ANIMALS)
    catalog = _candidate_constraints(target, difficulty, locale)

    # Four animals and four stalls: exhaustive enumeration is small (24).
    # Find the first smallest clue subset giving a unique arrangement.
    from itertools import combinations

    for size in range(1, len(catalog) + 1):
        for subset in combinations(catalog, size):
            constraints = (uniqueness,) + tuple(c for c, _ in subset)
            problem = Problem(variables, constraints)
            solutions = solve(problem, limit=2)
            if len(solutions) == 1:
                actual = tuple(solutions[0].values[:4])
                if actual != target:
                    continue
                return ArkLevel(
                    difficulty,
                    problem,
                    actual,
                    tuple(text for _, text in subset),
                )

    raise RuntimeError("No uniquely solvable level found")


def check_level(level: ArkLevel, arrangement: tuple[str, ...]) -> bool:
    """Check the left-to-right animal order against the generated CSP."""
    if len(arrangement) != 4 or set(arrangement) != set(ANIMALS):
        return False

    positions = {animal: arrangement.index(animal) for animal in ANIMALS}
    values = tuple(positions[a] for a in ANIMALS) + STALLS
    return satisfies(level.problem, Assignment(values))
