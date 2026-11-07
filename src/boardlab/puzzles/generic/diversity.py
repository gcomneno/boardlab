"""Bounded, exact semantic diversity experiments for finite-domain CSP."""

from dataclasses import dataclass
from itertools import combinations

from boardlab.puzzles.generic.equivalence import equivalence_classes
from boardlab.puzzles.generic.generator import GenerationSpace
from boardlab.puzzles.generic.model import Problem


@dataclass(frozen=True)
class DiversityReport:
    """Exact classification over the enumerated candidate population."""

    candidates: int
    semantic_classes: int
    class_sizes: tuple[int, ...]
    diversity_ratio: float


def measure_diversity(
    space: GenerationSpace,
    *,
    min_constraints: int = 0,
    max_constraints: int | None = None,
    max_candidates: int = 256,
    max_variables: int = 6,
    max_assignments: int = 4096,
) -> DiversityReport:
    """Classify an exhaustive, budget-bounded constraint-subset population.

    A budget overrun raises ValueError: partial enumeration is never
    reported as an exhaustive diversity measurement.
    """
    if type(min_constraints) is not int or min_constraints < 0:
        raise ValueError("Invalid minimum constraint count")
    if max_constraints is not None and (
        type(max_constraints) is not int or max_constraints < min_constraints
    ):
        raise ValueError("Invalid maximum constraint count")
    if type(max_candidates) is not int or max_candidates < 1:
        raise ValueError("Invalid candidate budget")

    upper = min(
        len(space.constraints),
        len(space.constraints) if max_constraints is None else max_constraints,
    )

    problems: list[Problem] = []

    for size in range(min_constraints, upper + 1):
        for selected in combinations(space.constraints, size):
            if len(problems) >= max_candidates:
                raise ValueError("Candidate budget exceeded")

            problems.append(Problem(space.variables, selected))

    if not problems:
        return DiversityReport(0, 0, (), 0.0)

    classes = equivalence_classes(
        tuple(problems),
        max_variables=max_variables,
        max_assignments=max_assignments,
    )
    sizes = tuple(len(group) for group in classes)

    return DiversityReport(
        candidates=len(problems),
        semantic_classes=len(classes),
        class_sizes=sizes,
        diversity_ratio=len(classes) / len(problems),
    )
