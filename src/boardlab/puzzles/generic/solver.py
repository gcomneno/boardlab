"""Deterministic finite-domain CSP solver with partial-assignment pruning."""

from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    ConstraintKind,
    Problem,
    Value,
)


def _same(left: Value, right: Value) -> bool:
    """Compare values without implicit conversions between their types."""
    return type(left) is type(right) and left == right


def _consistent(
    problem: Problem,
    values: tuple[Value | None, ...],
) -> bool:
    """Check that every constraint can still be satisfied."""
    indices = {variable.name: index for index, variable in enumerate(problem.variables)}

    for constraint in problem.constraints:
        if not _constraint_possible(problem, constraint, indices, values):
            return False

    return True


def _constraint_possible(
    problem: Problem,
    constraint: Constraint,
    indices: dict[str, int],
    values: tuple[Value | None, ...],
) -> bool:
    """Test one constraint, including limited domain lookahead."""
    selected = tuple(values[indices[name]] for name in constraint.variables)

    if constraint.kind is ConstraintKind.EQUAL:
        left, right = selected
        if left is not None and right is not None:
            return _same(left, right)
        if left is None and right is None:
            return True
        known = left if left is not None else right
        missing_name = constraint.variables[0 if left is None else 1]
        missing_domain = problem.variables[indices[missing_name]].domain
        return any(known is not None and _same(known, candidate) for candidate in missing_domain)

    if constraint.kind is ConstraintKind.NOT_EQUAL:
        left, right = selected
        if left is not None and right is not None:
            return not _same(left, right)
        return True

    if constraint.kind is ConstraintKind.ALL_DIFFERENT:
        assigned = [value for value in selected if value is not None]
        return len(set(assigned)) == len(assigned)

    if constraint.kind is ConstraintKind.SUM_EQUALS:
        assert constraint.target is not None
        assigned_sum = sum(value for value in selected if type(value) is int)
        missing_domains = [
            problem.variables[indices[name]].domain
            for name, value in zip(constraint.variables, selected, strict=True)
            if value is None
        ]
        numeric_domains: list[tuple[int, ...]] = []
        for domain in missing_domains:
            if any(type(value) is not int for value in domain):
                raise ValueError("SUM_EQUALS requires integer domains")
            numeric_domains.append(tuple(value for value in domain if type(value) is int))

        minimum = assigned_sum + sum(min(domain) for domain in numeric_domains)
        maximum = assigned_sum + sum(max(domain) for domain in numeric_domains)
        return minimum <= constraint.target <= maximum

    raise ValueError("Unsupported constraint kind")


def satisfies(problem: Problem, assignment: Assignment) -> bool:
    """Check a complete assignment against all declared constraints."""
    assignment.validate(problem)
    if not assignment.is_complete(problem):
        return False
    return _consistent(problem, assignment.values)


def solve(
    problem: Problem,
    limit: int | None = None,
) -> tuple[Assignment, ...]:
    """Enumerate satisfying assignments in declared domain order."""
    if limit is not None and (type(limit) is not int or limit < 1):
        raise ValueError("Limit must be a positive integer")

    solutions: list[Assignment] = []
    values: list[Value | None] = [None] * len(problem.variables)

    def search(index: int) -> None:
        if limit is not None and len(solutions) >= limit:
            return

        if index == len(problem.variables):
            candidate = Assignment(tuple(values))
            if _consistent(problem, candidate.values):
                solutions.append(candidate)
            return

        for value in problem.variables[index].domain:
            values[index] = value

            if _consistent(problem, tuple(values)):
                search(index + 1)

            values[index] = None

            if limit is not None and len(solutions) >= limit:
                return

    search(0)
    return tuple(solutions)


def count_solutions(problem: Problem, limit: int | None = 2) -> int:
    """Count solutions up to the limit, or exhaustively when None."""
    return len(solve(problem, limit=limit))
