#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def repository_files() -> list[Path]:
    result = subprocess.run(
        [
            "git",
            "ls-files",
            "-z",
            "--cached",
            "--others",
            "--exclude-standard",
        ],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )

    return [ROOT / raw.decode("utf-8") for raw in result.stdout.split(b"\0") if raw]


def is_probably_binary(data: bytes) -> bool:
    return b"\0" in data


def main() -> int:
    errors: list[str] = []
    checked = 0
    skipped_binary = 0

    for path in repository_files():
        if not path.is_file():
            continue

        data = path.read_bytes()

        if is_probably_binary(data):
            skipped_binary += 1
            continue

        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            skipped_binary += 1
            continue

        checked += 1
        relative = path.relative_to(ROOT)

        if data and not data.endswith(b"\n"):
            errors.append(f"{relative}: newline finale mancante")

        if "\r" in text:
            errors.append(f"{relative}: carattere CR rilevato; atteso LF")

        lines = text.splitlines()

        for number, line in enumerate(lines, start=1):
            if line.endswith((" ", "\t")):
                errors.append(f"{relative}:{number}: whitespace finale rilevato")

    if errors:
        print("CONTROLLO WHITESPACE REPOSITORY-WIDE FALLITO")
        for error in errors:
            print(f"- {error}")
        return 1

    print("CONTROLLO WHITESPACE REPOSITORY-WIDE SUPERATO")
    print(f"File di testo verificati: {checked}")
    print(f"File binari/non UTF-8 ignorati: {skipped_binary}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
