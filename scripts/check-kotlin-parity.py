#!/usr/bin/env python3
"""Verify Kotlin domain parity against versioned Python fixtures."""

import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FIXTURE = ROOT / "tests/fixtures/tre_sigilli/v1.json"
KOTLIN = ROOT / "kotlin/tre-sigilli/src"
DEFAULT_COMPILER = ROOT / ".local-tools/kotlin-2.4.21/kotlinc/bin/kotlinc"


def cells_text(cells):
    return ",".join("." if cell is None else cell for cell in cells)


def move_text(move):
    if move["type"] == "place":
        return f"P{move['cell']}"
    if move["type"] == "slide":
        return f"S{move['origin']}:{move['destination']}"
    raise ValueError(f"Unknown move: {move}")


def export_cases(data, directory):
    cases = []
    transitions = []
    invalid = []

    for case in data["cases"]:
        state = case["state"]
        state_fields = [
            case["id"],
            str(state["turn"]),
            cells_text(state["cells"]),
        ]

        moves = ",".join(move_text(move) for move in case["legal_moves"])
        cases.append("\t".join([*state_fields, moves]))

        for entry in case["transitions"]:
            next_state = entry["next_state"]
            transitions.append(
                "\t".join(
                    [
                        *state_fields,
                        move_text(entry["move"]),
                        str(next_state["turn"]),
                        cells_text(next_state["cells"]),
                    ]
                )
            )

        for entry in case["invalid_moves"]:
            if entry["error"] != "illegal_move":
                raise ValueError(f"Unexpected error: {entry}")
            invalid.append("\t".join([*state_fields, move_text(entry["move"])]))

    for name, lines in (
        ("cases.tsv", cases),
        ("transitions.tsv", transitions),
        ("invalid-moves.tsv", invalid),
    ):
        (directory / name).write_text("\n".join(lines) + "\n")

    print(f"ORACLE_CASES={len(cases)}", flush=True)
    print(f"ORACLE_TRANSITIONS={len(transitions)}", flush=True)
    print(f"ORACLE_INVALID_MOVES={len(invalid)}", flush=True)


def main():
    requested = os.environ.get("KOTLINC", str(DEFAULT_COMPILER))
    resolved = shutil.which(requested)
    if resolved is None:
        raise SystemExit(f"KOTLIN_COMPILER_MISSING={requested}")
    compiler = Path(resolved)

    data = json.loads(FIXTURE.read_text())
    if data["schema_version"] != 1 or data["game"] != "tre_sigilli":
        raise SystemExit("UNSUPPORTED_PARITY_SCHEMA")

    sources = [
        KOTLIN / "main/kotlin/boardlab/tre_sigilli/Model.kt",
        KOTLIN / "main/kotlin/boardlab/tre_sigilli/Rules.kt",
        KOTLIN / "main/kotlin/boardlab/tre_sigilli/Territory.kt",
        KOTLIN / "main/kotlin/boardlab/tre_sigilli/AlphaBeta.kt",
        KOTLIN / "main/kotlin/boardlab/tre_sigilli/GameController.kt",
        KOTLIN / "test/kotlin/boardlab/tre_sigilli/ParityTest.kt",
    ]

    with tempfile.TemporaryDirectory(prefix="boardlab-kotlin-") as temp:
        directory = Path(temp)
        export_cases(data, directory)
        jar = directory / "parity.jar"

        subprocess.run(
            [
                str(compiler),
                *(str(source) for source in sources),
                "-include-runtime",
                "-d",
                str(jar),
            ],
            check=True,
        )
        subprocess.run(
            [
                "java",
                "-jar",
                str(jar),
                str(directory / "cases.tsv"),
                str(directory / "transitions.tsv"),
                str(directory / "invalid-moves.tsv"),
            ],
            check=True,
        )

        controller_test = KOTLIN / "test/kotlin/boardlab/tre_sigilli/GameControllerTest.kt"
        controller_jar = directory / "controller-test.jar"
        subprocess.run(
            [
                str(compiler),
                *(
                    str(KOTLIN / f"main/kotlin/boardlab/tre_sigilli/{name}")
                    for name in (
                        "Model.kt",
                        "Rules.kt",
                        "Territory.kt",
                        "AlphaBeta.kt",
                        "GameController.kt",
                    )
                ),
                str(controller_test),
                "-include-runtime",
                "-d",
                str(controller_jar),
            ],
            check=True,
        )
        subprocess.run(["java", "-jar", str(controller_jar)], check=True)

    print("KOTLIN_REPRODUCIBLE_PARITY=PASS", flush=True)
    print("KOTLIN_CONTROLLER_CI=PASS", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"KOTLIN_REPRODUCIBLE_PARITY=FAIL: {error}", file=sys.stderr)
        sys.exit(1)
