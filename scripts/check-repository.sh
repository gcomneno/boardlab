#!/usr/bin/env bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$ROOT" || {
    printf '%s\n' "ERRORE: impossibile entrare nel repository."
    exit 1
}

FAILED=0

run_check() {
    label="$1"
    shift

    printf '\n===== %s =====\n' "$label"

    if "$@"; then
        printf 'OK: %s\n' "$label"
    else
        status="$?"
        printf 'ERRORE: %s (exit=%s)\n' "$label" "$status"
        FAILED=1
    fi
}

run_check \
    "RUFF FORMAT" \
    uv run ruff format --check .

run_check \
    "RUFF LINT" \
    uv run ruff check .

run_check \
    "MYPY" \
    uv run mypy

run_check \
    "PYTEST" \
    uv run pytest

run_check \
    "BILINGUAL DOCS" \
    python3 scripts/check-bilingual-docs.py

run_check \
    "SESSION 01 READINESS" \
    python3 scripts/check-session-01-readiness.py

run_check \
    "PUBLIC CONTENT" \
    scripts/check-public-content.sh

run_check \
    "SHELL SYNTAX: check-public-content.sh" \
    bash -n scripts/check-public-content.sh

run_check \
    "SHELL SYNTAX: check-repository.sh" \
    bash -n scripts/check-repository.sh

run_check \
    "WHITESPACE REPOSITORY-WIDE" \
    python3 scripts/check-whitespace.py

run_check \
    "WHITESPACE DIFF" \
    git diff --check

if [ "$FAILED" -ne 0 ]; then
    printf '\n%s\n' "VALIDAZIONE REPOSITORY FALLITA"
    exit 1
fi

printf '\n%s\n' "VALIDAZIONE REPOSITORY SUPERATA"
