"""Deterministic generation of finite-domain CSP instances."""

from dataclasses import dataclass
from enum import Enum
from itertools import combinations

from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    Problem,
    Variable,
)
from boardlab.puzzles.generic.solver import solve


@dataclass(frozen=True)
class GenerationSpace:
    """Variables and ordered catalog of admissible constraints."""

    variables: tuple[Variable, ...]
    constraints: tuple[Constraint, ...]

    def __post_init__(self) -> None:
        if not isinstance(self.constraints, tuple):
            raise TypeError("Constraints must be a tuple")

        # Reuse the canonical structural validation contract.
        Problem(self.variables, self.constraints)

        if len(set(self.constraints)) != len(self.constraints):
            raise ValueError("Duplicate candidate constraints")


@dataclass(frozen=True)
class GenerationPolicy:
    """Acceptance criteria and maximum exploration effort."""

    required_solutions: int = 1
    min_constraints: int = 1
    max_constraints: int | None = None
    max_candidates: int | None = None

    def __post_init__(self) -> None:
        if type(self.required_solutions) is not int or self.required_solutions < 0:
            raise ValueError("required_solutions must be nonnegative")

        if type(self.min_constraints) is not int or self.min_constraints < 0:
            raise ValueError("min_constraints must be nonnegative")

        if self.max_constraints is not None:
            if type(self.max_constraints) is not int:
                raise ValueError("max_constraints must be an integer")
            if self.max_constraints < self.min_constraints:
                raise ValueError("max_constraints cannot be below min_constraints")

        if self.max_candidates is not None and (
            type(self.max_candidates) is not int or self.max_candidates < 1
        ):
            raise ValueError("max_candidates must be positive")


class GenerationOutcome(Enum):
    FOUND = "found"
    EXHAUSTED = "exhausted"
    BUDGET_EXHAUSTED = "budget_exhausted"


@dataclass(frozen=True)
class GenerationResult:
    """Certified result, including explicit search termination status."""

    outcome: GenerationOutcome
    problem: Problem | None
    solutions: tuple[Assignment, ...]
    candidates_checked: int

    @property
    def found(self) -> bool:
        return self.outcome is GenerationOutcome.FOUND


def generate(
    space: GenerationSpace,
    policy: GenerationPolicy | None = None,
) -> GenerationResult:
    """Select the first candidate with exactly the required solution count.

    Search is deterministic: smaller subsets first, preserving the
    declaration order of the constraint catalog.

    A cardinality check enumerates at most required_solutions + 1
    solutions. The extra solution distinguishes an exact count from
    an exceeded count.
    """
    if policy is None:
        policy = GenerationPolicy()

    upper = min(
        len(space.constraints),
        policy.max_constraints if policy.max_constraints is not None else len(space.constraints),
    )
    checked = 0

    for size in range(policy.min_constraints, upper + 1):
        for selected in combinations(space.constraints, size):
            if policy.max_candidates is not None and checked >= policy.max_candidates:
                return GenerationResult(
                    GenerationOutcome.BUDGET_EXHAUSTED,
                    None,
                    (),
                    checked,
                )

            candidate = Problem(space.variables, selected)
            checked += 1

            solutions = solve(
                candidate,
                limit=policy.required_solutions + 1,
            )
            if len(solutions) == policy.required_solutions:
                return GenerationResult(
                    GenerationOutcome.FOUND,
                    candidate,
                    solutions,
                    checked,
                )

    return GenerationResult(
        GenerationOutcome.EXHAUSTED,
        None,
        (),
        checked,
    )
