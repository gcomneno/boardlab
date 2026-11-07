"""L'Arca di Darwin — ecological survival game.

The generic CSP solver certifies viable embarkation configurations.
Environmental events and ecological scoring remain game-specific.
"""

from dataclasses import dataclass
from itertools import permutations
from typing import Any

from boardlab.puzzles.generic.model import (
    Assignment,
    Constraint,
    ConstraintKind,
    Problem,
    Variable,
)
from boardlab.puzzles.generic.solver import satisfies, solve


@dataclass(frozen=True)
class Species:
    name: str
    description: str
    emoji: str
    food: int
    water: int
    habitat: str
    role: str


SPECIES = (
    Species("leone", "Predatore territoriale", "🦁", 3, 2, "savana", "predatore"),
    Species("zebra", "Erbivoro di prateria", "🦓", 2, 2, "savana", "erbivoro"),
    Species("elefante", "Grande ingegnere dell'ecosistema", "🐘", 3, 4, "savana", "ingegnere"),
    Species("coniglio", "Piccolo erbivoro prolifico", "🐇", 1, 1, "prateria", "erbivoro"),
    Species("lupo", "Predatore sociale", "🐺", 3, 2, "foresta", "predatore"),
    Species("tartaruga", "Rettilе resistente", "🐢", 1, 1, "palude", "resistente"),
    Species("volpe", "Predatore opportunista", "🦊", 2, 2, "foresta", "predatore"),
    Species("giraffa", "Grande erbivoro della savana", "🦒", 3, 3, "savana", "erbivoro"),
    Species("castoro", "Ingegnere delle zone umide", "🦫", 2, 2, "palude", "ingegnere"),
    Species("anatra", "Uccello acquatico onnivoro", "🦆", 1, 1, "palude", "onnivoro"),
)

SCENARIOS = (
    {
        "id": "stiva",
        "title": "Capitolo 1 — La Stiva dell'Arca",
        "description": "Il Diluvio sta arrivando. Prepara la stiva scegliendo quattro specie in grado di convivere. Gestisci le scorte di cibo e acqua, mantieni l'equilibrio tra gli animali e organizza una disposizione sicura.",
        "food": 9,
        "water": 9,
        "capacity": 4,
        "favored": "prateria",
        "event": "L'Arca supera la prima notte di tempesta.",
        "pigeon": "Cip cip! Ho controllato la stiva! Attenzione alle scorte e ai predatori!",
        "viable_count": 10,
    },
    {
        "id": "diluvio",
        "title": "Capitolo 2 — L'Allagamento",
        "description": "La tempesta ha raggiunto l'Arca e il primo compartimento è allagato. Alcune specie sono più adatte di altre ad affrontare queste condizioni. Riorganizza la stiva senza compromettere la sicurezza degli animali.",
        "food": 9,
        "water": 9,
        "capacity": 4,
        "favored": "palude",
        "event": "La tempesta è superata. L'Arca continua il viaggio.",
        "pigeon": "CIP! Il primo compartimento è allagato! Quale animale potrebbe affrontare questa situazione?",
        "viable_count": 10,
    },
    {
        "id": "savana",
        "title": "Capitolo 3 — La Savana in Siccità",
        "description": "La stagione secca ha ridotto drasticamente la disponibilità d'acqua. Hai soltanto 6 unità d'acqua per quattro animali. Scegli specie con consumi sostenibili e assicurati che possano convivere.",
        "food": 9,
        "water": 6,
        "capacity": 4,
        "favored": "savana",
        "event": "La siccità mette alla prova gli animali più assetati.",
        "pigeon": "Ho visto acqua a est! Attenzione alle specie che bevono molto.",
        "viable_count": 10,
    },
    {
        "id": "foresta",
        "title": "Capitolo 4 — Il Mosaico Forestale",
        "description": "La vegetazione si sta rigenerando, formando ambienti differenti. Per favorire la biodiversità devi riunire quattro specie appartenenti a quattro habitat differenti, mantenendo sostenibili le risorse.",
        "food": 10,
        "water": 10,
        "capacity": 4,
        "favored": "foresta",
        "event": "Una stagione piovosa favorisce la vegetazione.",
        "pigeon": "Ho esplorato gli alberi! Qui la biodiversità sarà preziosa.",
        "viable_count": 10,
    },
    {
        "id": "isole",
        "title": "Capitolo 5 — L'Arcipelago del Nuovo Mondo",
        "description": "L'Arca raggiunge un arcipelago di isole e zone umide. Le risorse sono limitate e almeno una specie dovrà essere capace di affrontare gli ambienti acquatici. Organizza la spedizione finale rispettando tutti i vincoli.",
        "food": 8,
        "water": 8,
        "capacity": 4,
        "favored": "palude",
        "event": "Le maree cambiano gli habitat disponibili.",
        "pigeon": "Dall'alto vedo tante isole! Servirà una scelta equilibrata.",
        "viable_count": 5,
    },
)


def scenario(identifier: str) -> dict[str, Any] | None:
    return next((s for s in SCENARIOS if s["id"] == identifier), None)


def build_problem(environment: dict[str, Any] | None = None) -> Problem:
    """Use the existing CSP to enforce the exact embarkation capacity."""
    environment = environment or SCENARIOS[0]
    names = tuple(s.name for s in SPECIES)
    capacity = environment["capacity"]
    if type(capacity) is not int:
        raise ValueError("Invalid environment capacity")
    return Problem(
        tuple(Variable(name, (0, 1)) for name in names),
        (Constraint(ConstraintKind.SUM_EQUALS, names, target=capacity),),
    )


def evaluate_selection(
    names: tuple[str, ...] | list[str], environment: dict[str, Any]
) -> dict[str, Any]:
    """Evaluate a complete selection using transparent ecological rules."""
    if not isinstance(names, (tuple, list)):
        return {"valid": False, "message": "Selezione non valida."}
    if len(names) != environment["capacity"] or len(set(names)) != len(names):
        return {"valid": False, "message": "Servono quattro specie diverse."}

    catalog = {s.name: s for s in SPECIES}
    if any(name not in catalog for name in names):
        return {"valid": False, "message": "Specie sconosciuta."}

    problem = build_problem(environment)
    assignment = Assignment(tuple(int(v.name in names) for v in problem.variables))
    if not satisfies(problem, assignment):
        return {"valid": False, "message": "Capacità dell'Arca non rispettata."}

    chosen = [catalog[name] for name in names]
    food = sum(s.food for s in chosen)
    water = sum(s.water for s in chosen)
    predators = sum(s.role == "predatore" for s in chosen)
    herbivores = sum(s.role == "erbivoro" for s in chosen)
    habitats = len({s.habitat for s in chosen})
    problems = []

    if food > environment["food"]:
        problems.append("Cibo insufficiente")
    if water > environment["water"]:
        problems.append("Acqua insufficiente")
    if predators > herbivores:
        problems.append("Troppi predatori rispetto agli erbivori")

    # Capitolo 4: almeno tre habitat distinti.
    if environment["id"] == "foresta" and habitats < 4:
        problems.append("Servono 4 habitat differenti")

    # Capitolo 5: la tartaruga deve essere imbarcata.
    if environment["id"] == "isole" and "tartaruga" not in names:
        problems.append("La tartaruga deve essere imbarcata")

    valid = not problems
    score = (
        (
            40
            + 10 * habitats
            + 10 * sum(s.habitat == environment["favored"] for s in chosen)
            + 5 * (environment["food"] - food)
            + 5 * (environment["water"] - water)
        )
        if valid
        else 0
    )

    return {
        "valid": valid,
        "score": score,
        "food": food,
        "water": water,
        "problems": problems,
        "message": (
            "Ecosistema sostenibile! Le specie possono iniziare una nuova vita."
            if valid
            else "L'ecosistema non è sostenibile: " + ", ".join(problems)
        ),
        "event": environment["event"],
    }


def viable_configurations(environment: dict[str, Any]) -> tuple[tuple[tuple[str, ...], int], ...]:
    """Enumerate viable configurations; capacity is CSP-certified."""
    problem = build_problem(environment)
    result = []
    for assignment in solve(problem):
        names = tuple(
            v.name
            for v, value in zip(problem.variables, assignment.values, strict=True)
            if value == 1
        )
        evaluation = evaluate_selection(names, environment)
        if evaluation["valid"]:
            result.append((names, evaluation["score"]))
    return tuple(result)


INCOMPATIBLE = (
    frozenset(("leone", "zebra")),
    frozenset(("lupo", "coniglio")),
)


def _hold_arrangements(names: tuple[str, ...] | list[str]) -> tuple[tuple[str, ...], ...]:
    """Enumerate compatible arrangements for the four-stall tutorial."""
    if len(names) != 4 or len(set(names)) != 4:
        raise ValueError("Exactly four distinct species required")
    if not set(names).issubset({s.name for s in SPECIES}):
        raise ValueError("Unknown species")

    return tuple(
        arrangement
        for arrangement in permutations(names)
        if all(
            frozenset((arrangement[i], arrangement[i + 1])) not in INCOMPATIBLE for i in range(3)
        )
    )


def hold_problem(names: tuple[str, ...] | list[str]) -> Problem:
    """CSP whose integer domain indexes compatible arrangements."""
    arrangements = _hold_arrangements(names)
    return Problem(
        (Variable("hold_arrangement", tuple(range(len(arrangements)))),),
        (),
    )


def hold_solutions(names: tuple[str, ...] | list[str]) -> tuple[tuple[str, ...], ...]:
    """Decode CSP solutions into complete hold arrangements."""
    arrangements = _hold_arrangements(names)
    problem = hold_problem(names)
    result: list[tuple[str, ...]] = []
    for assignment in solve(problem):
        index = assignment.values[0]
        if type(index) is not int:
            raise ValueError("Invalid hold solution index")
        result.append(arrangements[index])
    return tuple(result)


def validate_hold(
    arrangement: tuple[str, ...] | list[str], environment: dict[str, Any]
) -> dict[str, Any]:
    if not isinstance(arrangement, (list, tuple)):
        return {"valid": False, "message": "Disposizione non valida."}
    if len(arrangement) != 4 or len(set(arrangement)) != 4:
        return {"valid": False, "message": "Occorrono quattro animali diversi."}
    if not all(isinstance(x, str) for x in arrangement):
        return {"valid": False, "message": "Animali non validi."}
    if not set(arrangement).issubset({s.name for s in SPECIES}):
        return {"valid": False, "message": "Specie sconosciuta."}

    ecological = evaluate_selection(arrangement, environment)
    if not ecological["valid"]:
        return ecological

    if environment["id"] == "diluvio" and arrangement[0] != "tartaruga":
        ecological["valid"] = False
        ecological["score"] = 0
        ecological["message"] = (
            "Il primo compartimento è allagato! Qui può stare soltanto la tartaruga."
        )
        return ecological

    solutions = hold_solutions(arrangement)
    valid = tuple(arrangement) in solutions
    if not valid:
        ecological["valid"] = False
        ecological["score"] = 0
        ecological["message"] = (
            "Animali incompatibili in compartimenti vicini! "
            "Il piccione consiglia di cambiare la disposizione."
        )
    else:
        ecological["score"] += 20
        ecological["message"] = "Stiva sicura! Risorse equilibrate e animali ben separati."
    ecological["possible_arrangements"] = len(solutions)
    return ecological


def validate_selection(names: tuple[str, ...] | list[str]) -> bool:
    return bool(evaluate_selection(names, SCENARIOS[0])["valid"])


def solution() -> tuple[str, ...]:
    viable = viable_configurations(SCENARIOS[0])
    if not viable:
        raise ValueError("No viable configuration")
    return viable[0][0]


def scenario_is_unique() -> bool:
    return len(viable_configurations(SCENARIOS[0])) == 1
