# Development guide

[English](development.md) | [Italiano](development.it.md)

This guide explains how to prepare and validate BoardLab from a clean
repository clone.

## Requirements

- Git;
- Python 3.12 or later;
- `uv`.

Global installations of Ruff, mypy, or pytest are not required.

## Initial setup

From the directory where you want to store your projects:

```bash
git clone https://github.com/gcomneno/boardlab.git
cd boardlab
uv sync --group dev
```

`uv sync`:

1. creates `.venv` when necessary;
2. installs BoardLab as an editable package;
3. installs the development tools;
4. uses the versions recorded in `uv.lock`.

## Complete validation

Run from the repository root:

```bash
scripts/check-repository.sh
```

This is the canonical entry point for local validation. It runs both
the Python toolchain checks and repository-wide checks concerning
documentation and publishable content.

Validation includes at least:

- Ruff formatting;
- Ruff linting;
- mypy in strict mode;
- pytest;
- bilingual documentation validation;
- publishable-content checks;
- shell script syntax checks;
- Git whitespace checks.

All checks must succeed before creating a commit.

## Automatic formatting

When Ruff reports incorrectly formatted files:

```bash
uv run ruff format .
```

Repeat the complete validation after formatting.

## Updating dependencies

To intentionally update the lock file:

```bash
uv lock --upgrade
uv sync --group dev
```

The `uv.lock` diff must be reviewed and committed together with the
change requiring the update.

## Adding a runtime dependency

```bash
uv add nome-pacchetto
```

Runtime dependencies must be required for BoardLab to operate,
not merely for development.

## Adding a development dependency

```bash
uv add --dev nome-pacchetto
```

Testing, linting, typing, and maintenance tools belong to the
development dependency group.

## Pre-commit checks

First, run the complete validation:

```bash
scripts/check-repository.sh
```

Then explicitly check Git status:

```bash
git diff --check
git status -sb
```

`git diff` does not display untracked files. Always inspect
`git status` before considering the content of a commit complete.

Before the final commit for educational material preparation,
a separate staging audit is also required, limited to the
intended files.
