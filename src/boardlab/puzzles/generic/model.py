"""Immutable, declarative finite-domain CSP definitions."""

from dataclasses import dataclass
from enum import Enum

type Value = int | str


def _valid_value(value: object) -> bool:
    return type(value) in (int, str)


def _valid_name(value: object) -> bool:
    return type(value) is str and bool(value)


class ConstraintKind(Enum):
    """Supported declarative relations."""

    EQUAL = "equal"
    NOT_EQUAL = "not_equal"
    ALL_DIFFERENT = "all_different"
    SUM_EQUALS = "sum_equals"


@dataclass(frozen=True)
class Variable:
    """A named variable with an ordered finite domain."""

    name: str
    domain: tuple[Value, ...]

    def __post_init__(self) -> None:
        if not _valid_name(self.name):
            raise ValueError("Invalid variable name")
        if type(self.domain) is not tuple or not self.domain:
            raise ValueError("Domain must be a nonempty tuple")
        if any(not _valid_value(value) for value in self.domain):
            raise ValueError("Invalid domain value")
        if len(set(self.domain)) != len(self.domain):
            raise ValueError("Duplicate domain values")


@dataclass(frozen=True)
class Constraint:
    """Declarative relation over named variables."""

    kind: ConstraintKind
    variables: tuple[str, ...]
    target: int | None = None

    def __post_init__(self) -> None:
        if not isinstance(self.kind, ConstraintKind):
            raise ValueError("Invalid constraint kind")
        if type(self.variables) is not tuple:
            raise ValueError("Constraint variables must be a tuple")
        if any(not _valid_name(name) for name in self.variables):
            raise ValueError("Invalid constraint variable")
        if len(set(self.variables)) != len(self.variables):
            raise ValueError("Duplicate constraint variables")

        if self.kind in (ConstraintKind.EQUAL, ConstraintKind.NOT_EQUAL):
            if len(self.variables) != 2 or self.target is not None:
                raise ValueError("Invalid binary constraint")
        elif self.kind is ConstraintKind.ALL_DIFFERENT:
            if len(self.variables) < 2 or self.target is not None:
                raise ValueError("Invalid all-different constraint")
        elif not self.variables or type(self.target) is not int:
            raise ValueError("Invalid sum-equals constraint")


@dataclass(frozen=True)
class Problem:
    """A structurally valid finite-domain constraint problem."""

    variables: tuple[Variable, ...]
    constraints: tuple[Constraint, ...] = ()

    def __post_init__(self) -> None:
        if type(self.variables) is not tuple or not self.variables:
            raise ValueError("Variables must be a nonempty tuple")
        if any(not isinstance(v, Variable) for v in self.variables):
            raise ValueError("Invalid variable")
        if type(self.constraints) is not tuple:
            raise ValueError("Constraints must be a tuple")
        if any(not isinstance(c, Constraint) for c in self.constraints):
            raise ValueError("Invalid constraint")

        names = tuple(variable.name for variable in self.variables)
        if len(set(names)) != len(names):
            raise ValueError("Duplicate variable names")

        known = set(names)
        numeric = {
            variable.name
            for variable in self.variables
            if all(type(value) is int for value in variable.domain)
        }

        for constraint in self.constraints:
            if any(name not in known for name in constraint.variables):
                raise ValueError("Unknown constraint variable")
            if (
                constraint.kind is ConstraintKind.SUM_EQUALS
                and not set(constraint.variables) <= numeric
            ):
                raise ValueError("SUM_EQUALS requires integer domains")


@dataclass(frozen=True)
class Assignment:
    """Complete or partial values in problem variable order."""

    values: tuple[Value | None, ...]

    def validate(self, problem: Problem) -> None:
        if type(self.values) is not tuple:
            raise ValueError("Assignment values must be a tuple")
        if len(self.values) != len(problem.variables):
            raise ValueError("Incorrect assignment size")

        for value, variable in zip(self.values, problem.variables, strict=True):
            if value is None:
                continue
            if not _valid_value(value) or value not in variable.domain:
                raise ValueError("Assignment value outside domain")

    def is_complete(self, problem: Problem) -> bool:
        self.validate(problem)
        return all(value is not None for value in self.values)
