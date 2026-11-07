"""Exact semantic equivalence for finite-domain CSP problems.

Research baseline: fixed variable names, fixed typed domain values.
No variable or value renaming is assumed.
"""

from __future__ import annotations

from boardlab.puzzles.generic.model import Problem
from boardlab.puzzles.generic.solver import solve


def semantically_equivalent(left: Problem, right: Problem) -> bool:
    """Compare complete solution sets under fixed variable identities."""
    left_domains = {v.name: frozenset(v.domain) for v in left.variables}
    right_domains = {v.name: frozenset(v.domain) for v in right.variables}

    if left_domains != right_domains:
        return False

    names = tuple(sorted(left_domains))

    def solution_set(problem: Problem) -> set[tuple[int | str, ...]]:
        indices = {variable.name: index for index, variable in enumerate(problem.variables)}
        result: set[tuple[int | str, ...]] = set()
        for assignment in solve(problem, limit=None):
            values = tuple(assignment.values[indices[name]] for name in names)
            if any(value is None for value in values):
                raise ValueError("Incomplete solver assignment")
            result.add(tuple(value for value in values if value is not None))
        return result

    return solution_set(left) == solution_set(right)


def equivalent_under_renaming(
    left: Problem,
    right: Problem,
    *,
    max_variables: int = 6,
    max_assignments: int = 4096,
) -> bool:
    """Exact equivalence up to a variable-name bijection.

    Domain values remain fixed. Only variable identities may change.
    Raises ValueError when the research exploration budget is exceeded.
    """
    from itertools import permutations
    from math import prod

    if type(max_variables) is not int or max_variables < 1:
        raise ValueError("Invalid variable budget")
    if type(max_assignments) is not int or max_assignments < 1:
        raise ValueError("Invalid assignment budget")

    if len(left.variables) != len(right.variables):
        return False

    n = len(left.variables)
    if n > max_variables:
        raise ValueError("Variable budget exceeded")

    for problem in (left, right):
        size = prod(len(variable.domain) for variable in problem.variables)
        if size > max_assignments:
            raise ValueError("Assignment budget exceeded")

    left_domains = tuple(frozenset(variable.domain) for variable in left.variables)
    left_solutions = {assignment.values for assignment in solve(left, limit=None)}

    right_domains = tuple(frozenset(variable.domain) for variable in right.variables)
    right_solutions = {assignment.values for assignment in solve(right, limit=None)}

    # A permutation specifies which right-hand variable corresponds
    # to each left-hand variable. It cannot relabel domain values.
    for positions in permutations(range(n)):
        if any(left_domains[i] != right_domains[positions[i]] for i in range(n)):
            continue

        transformed = {
            tuple(values[position] for position in positions) for values in right_solutions
        }

        if left_solutions == transformed:
            return True

    return False


def equivalence_classes(
    problems: tuple[Problem, ...],
    *,
    max_variables: int = 6,
    max_assignments: int = 4096,
) -> tuple[tuple[int, ...], ...]:
    """Group input indices by exact equivalence under renaming.

    The first occurrence represents its class. This is an exhaustive
    reference implementation, not a scalable canonicalization.
    """
    classes: list[list[int]] = []

    for index, problem in enumerate(problems):
        for group in classes:
            if equivalent_under_renaming(
                problems[group[0]],
                problem,
                max_variables=max_variables,
                max_assignments=max_assignments,
            ):
                group.append(index)
                break
        else:
            # Validate the budget even for singleton classes.
            equivalent_under_renaming(
                problem,
                problem,
                max_variables=max_variables,
                max_assignments=max_assignments,
            )
            classes.append([index])

    return tuple(tuple(group) for group in classes)
