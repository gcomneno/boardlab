#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SESSION = ROOT / "sessions/session-01-modeling-a-board-game"

EXPECTED_UNITS = (
    "unit-01-game-state",
    "unit-02-legal-moves",
    "unit-03-state-transitions",
    "unit-04-terminal-states-and-outcome",
    "unit-05-player-and-strategy",
    "unit-06-match-coordination",
)

EXPECTED_QUIZZES = (
    "quiz-01-state-legality-transitions",
    "quiz-02-termination-strategy-coordination",
)

EXPECTED_TOP_LEVEL = (
    "README",
    "source-coverage-map",
    "study-map",
    "quiz-plan",
    "coverage-review",
)

QUIZ_LEAKAGE_PATTERNS = (
    r"\bcorrect answer\b",
    r"\brisposta corretta\b",
    r"\banswer key\b",
    r"\bsolution\b",
    r"\bsoluzione\b",
)

EXPECTED_SESSION_NAVIGATION = (
    "source-coverage-map.md",
    "study-map.md",
    "quiz-plan.md",
    "coverage-review.md",
    "units/unit-01-game-state.md",
    "units/unit-02-legal-moves.md",
    "units/unit-03-state-transitions.md",
    "units/unit-04-terminal-states-and-outcome.md",
    "units/unit-05-player-and-strategy.md",
    "units/unit-06-match-coordination.md",
    "quizzes/quiz-01-state-legality-transitions.md",
    "quizzes/quiz-02-termination-strategy-coordination.md",
    "answer-keys/quiz-01-state-legality-transitions.md",
    "answer-keys/quiz-02-termination-strategy-coordination.md",
)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def pair_paths(directory: Path, stem: str) -> tuple[Path, Path]:
    return directory / f"{stem}.md", directory / f"{stem}.it.md"


def check_file_pair(
    errors: list[str],
    directory: Path,
    stem: str,
    label: str,
) -> None:
    canonical, translation = pair_paths(directory, stem)

    if not canonical.is_file():
        errors.append(f"{label}: file canonico mancante: {canonical.relative_to(ROOT)}")

    if not translation.is_file():
        errors.append(f"{label}: traduzione italiana mancante: {translation.relative_to(ROOT)}")


def check_expected_inventory(errors: list[str]) -> None:
    for stem in EXPECTED_TOP_LEVEL:
        check_file_pair(errors, SESSION, stem, f"top-level {stem}")

    for stem in EXPECTED_UNITS:
        check_file_pair(errors, SESSION / "units", stem, f"unit {stem}")

    for stem in EXPECTED_QUIZZES:
        check_file_pair(errors, SESSION / "quizzes", stem, f"quiz {stem}")
        check_file_pair(errors, SESSION / "answer-keys", stem, f"answer key {stem}")

    unit_files = {path.name for path in (SESSION / "units").glob("unit-*.md") if path.is_file()}
    expected_unit_files = {f"{stem}.md" for stem in EXPECTED_UNITS} | {
        f"{stem}.it.md" for stem in EXPECTED_UNITS
    }

    if unit_files != expected_unit_files:
        missing = sorted(expected_unit_files - unit_files)
        extra = sorted(unit_files - expected_unit_files)

        if missing:
            errors.append(f"unit inventory: file mancanti: {', '.join(missing)}")

        if extra:
            errors.append(f"unit inventory: file inattesi: {', '.join(extra)}")

    quiz_files = {path.name for path in (SESSION / "quizzes").glob("quiz-*.md") if path.is_file()}
    expected_quiz_files = {f"{stem}.md" for stem in EXPECTED_QUIZZES} | {
        f"{stem}.it.md" for stem in EXPECTED_QUIZZES
    }

    if quiz_files != expected_quiz_files:
        missing = sorted(expected_quiz_files - quiz_files)
        extra = sorted(quiz_files - expected_quiz_files)

        if missing:
            errors.append(f"quiz inventory: file mancanti: {', '.join(missing)}")

        if extra:
            errors.append(f"quiz inventory: file inattesi: {', '.join(extra)}")

    answer_key_files = {
        path.name for path in (SESSION / "answer-keys").glob("quiz-*.md") if path.is_file()
    }

    if answer_key_files != expected_quiz_files:
        missing = sorted(expected_quiz_files - answer_key_files)
        extra = sorted(answer_key_files - expected_quiz_files)

        if missing:
            errors.append(f"answer-key inventory: file mancanti: {', '.join(missing)}")

        if extra:
            errors.append(f"answer-key inventory: file inattesi: {', '.join(extra)}")


def count_headings(text: str, prefix: str) -> int:
    return sum(1 for line in text.splitlines() if line.startswith(prefix))


def check_quizzes(errors: list[str]) -> None:
    for stem in EXPECTED_QUIZZES:
        canonical, translation = pair_paths(SESSION / "quizzes", stem)

        if not canonical.is_file() or not translation.is_file():
            continue

        canonical_text = read_text(canonical)
        translation_text = read_text(translation)

        canonical_count = count_headings(canonical_text, "## Question ")
        translation_count = count_headings(translation_text, "## Domanda ")

        if canonical_count != 6:
            errors.append(
                f"{canonical.relative_to(ROOT)}: attese 6 domande, trovate {canonical_count}"
            )

        if translation_count != 6:
            errors.append(
                f"{translation.relative_to(ROOT)}: attese 6 domande, trovate {translation_count}"
            )

        for path, text in (
            (canonical, canonical_text),
            (translation, translation_text),
        ):
            lowered = text.casefold()

            for pattern in QUIZ_LEAKAGE_PATTERNS:
                if re.search(pattern, lowered, flags=re.IGNORECASE):
                    errors.append(f"{path.relative_to(ROOT)}: possibile leakage quiz: {pattern}")


def check_answer_keys(errors: list[str]) -> None:
    for stem in EXPECTED_QUIZZES:
        canonical, translation = pair_paths(SESSION / "answer-keys", stem)

        if not canonical.is_file() or not translation.is_file():
            continue

        canonical_count = count_headings(read_text(canonical), "## Question ")
        translation_count = count_headings(read_text(translation), "## Domanda ")

        if canonical_count != 6:
            errors.append(
                f"{canonical.relative_to(ROOT)}: attese 6 risposte, trovate {canonical_count}"
            )

        if translation_count != 6:
            errors.append(
                f"{translation.relative_to(ROOT)}: attese 6 risposte, trovate {translation_count}"
            )


def check_content_result(errors: list[str]) -> None:
    canonical, translation = pair_paths(SESSION, "coverage-review")

    for path in (canonical, translation):
        if not path.is_file():
            continue

        if "**Content preparation complete.**" not in read_text(path):
            errors.append(f"{path.relative_to(ROOT)}: marker Content preparation complete mancante")


def check_navigation(errors: list[str]) -> None:
    canonical = SESSION / "README.md"
    translation = SESSION / "README.it.md"

    if canonical.is_file():
        text = read_text(canonical)

        for target in EXPECTED_SESSION_NAVIGATION:
            if target not in text:
                errors.append(
                    f"{canonical.relative_to(ROOT)}: link di navigazione mancante: {target}"
                )

    if translation.is_file():
        text = read_text(translation)

        for target in EXPECTED_SESSION_NAVIGATION:
            italian_target = target[:-3] + ".it.md" if target.endswith(".md") else target

            if italian_target not in text:
                errors.append(
                    f"{translation.relative_to(ROOT)}: link di navigazione "
                    f"mancante: {italian_target}"
                )


def check_study_state(errors: list[str]) -> None:
    progress_files = (
        ROOT / "docs/progress.md",
        ROOT / "docs/progress.it.md",
        SESSION / "README.md",
        SESSION / "README.it.md",
    )

    for path in progress_files:
        if not path.is_file():
            errors.append(f"study state: file mancante: {path.relative_to(ROOT)}")
            continue

        text = read_text(path)

        if "**not studied**" not in text:
            errors.append(f"{path.relative_to(ROOT)}: stato learner 'not studied' mancante")


def check_prepared_state(errors: list[str], require_prepared: bool) -> None:
    if not require_prepared:
        return

    required_markers = (
        (
            ROOT / "sessions/README.md",
            "Repository preparation: **Prepared**.",
        ),
        (
            ROOT / "sessions/README.it.md",
            "Preparazione repository: **Prepared**.",
        ),
        (
            ROOT / "docs/progress.md",
            "Repository preparation: **Prepared**.",
        ),
        (
            ROOT / "docs/progress.it.md",
            "Preparazione repository: **Prepared**.",
        ),
        (
            SESSION / "README.md",
            "Repository preparation: **Prepared**.",
        ),
        (
            SESSION / "README.it.md",
            "Preparazione repository: **Prepared**.",
        ),
    )

    for marker_path, marker in required_markers:
        if not marker_path.is_file():
            errors.append(f"Prepared state: file mancante: {marker_path.relative_to(ROOT)}")
            continue

        content = read_text(marker_path)

        if marker not in content:
            errors.append(
                f"{marker_path.relative_to(ROOT)}: marker di stato Prepared mancante: {marker}"
            )

        if "**not studied**" not in content:
            errors.append(f"{marker_path.relative_to(ROOT)}: stato learner 'not studied' mancante")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate BoardLab Session 01 preparation readiness."
    )
    parser.add_argument(
        "--require-prepared",
        action="store_true",
        help="Require the final Prepared status markers.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    errors: list[str] = []

    check_expected_inventory(errors)
    check_quizzes(errors)
    check_answer_keys(errors)
    check_content_result(errors)
    check_navigation(errors)
    check_study_state(errors)
    check_prepared_state(errors, args.require_prepared)

    if errors:
        print("READINESS SESSIONE 01 FALLITA")
        for error in errors:
            print(f"- {error}")
        return 1

    print("READINESS SESSIONE 01 SUPERATA")
    print("Unità: 6 EN + 6 IT")
    print("Quiz: 2 EN + 2 IT, 6 domande ciascuno")
    print("Answer key: 2 EN + 2 IT, 6 risposte ciascuna")
    print("Quiz leakage: assente")
    print("Content preparation: complete")
    print("Learner state: not studied")

    if args.require_prepared:
        print("Repository preparation: Prepared")

    return 0


if __name__ == "__main__":
    sys.exit(main())
