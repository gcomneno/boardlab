#!/usr/bin/env python3
"""Regression tests for the BoardLab bilingual Markdown validator."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR_PATH = ROOT / "scripts" / "check-bilingual-docs.py"
FENCE_CHR = chr(96)

SPEC = importlib.util.spec_from_file_location(
    "bilingual_docs_validator",
    VALIDATOR_PATH,
)

if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"Unable to load {VALIDATOR_PATH}")

validator = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = validator
SPEC.loader.exec_module(validator)


class BilingualValidatorTests(unittest.TestCase):
    def parse_sample(self, content: str) -> Any:
        with tempfile.TemporaryDirectory(
            prefix=".bilingual-docs-test-",
            dir=ROOT,
        ) as temporary:
            path = Path(temporary) / "sample.md"
            path.write_text(content, encoding="utf-8")

            errors: list[str] = []
            document = validator.parse_markdown(
                path.resolve(),
                errors,
            )

        self.assertEqual(errors, [])
        return document

    def make_pair(
        self,
        directory: Path,
        english_command: str,
        italian_command: str,
    ) -> tuple[Path, Path]:
        canonical = directory / "sample.md"
        translation = directory / "sample.it.md"
        navigation = "[English](sample.md) | [Italiano](sample.it.md)"

        canonical.write_text(
            (f"# Sample\n\n{navigation}\n\n    {english_command}\n"),
            encoding="utf-8",
        )

        translation.write_text(
            (f"# Esempio\n\n{navigation}\n\n    {italian_command}\n"),
            encoding="utf-8",
        )

        return canonical, translation

    def test_fenced_and_indented_code_order(self) -> None:
        fence = FENCE_CHR * 3

        document = self.parse_sample(
            f"# Probe\n\n{fence}bash\necho fenced\n{fence}\n\n    echo indented\n"
        )

        self.assertEqual(
            document.code_blocks,
            (
                ("bash", "echo fenced"),
                ("", "echo indented"),
            ),
        )

    def test_pair_rejects_different_code(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix=".bilingual-docs-test-",
            dir=ROOT,
        ) as temporary:
            canonical, translation = self.make_pair(
                Path(temporary),
                "python3 probe.py",
                "python3 probe.py --verbose",
            )

            errors: list[str] = []
            validator.validate_pair(
                canonical,
                translation,
                ".it.md",
                errors,
            )

        self.assertTrue(
            any("blocchi di codice differenti" in error for error in errors),
            errors,
        )

    def test_pair_accepts_equivalent_code(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix=".bilingual-docs-test-",
            dir=ROOT,
        ) as temporary:
            canonical, translation = self.make_pair(
                Path(temporary),
                "python3 probe.py",
                "python3 probe.py",
            )

            errors: list[str] = []
            validator.validate_pair(
                canonical,
                translation,
                ".it.md",
                errors,
            )

        self.assertEqual(errors, [])

    def test_repository_hygiene_pair(self) -> None:
        errors: list[str] = []

        validator.validate_pair(
            ROOT / "docs" / "REPO-HYGIENE.md",
            ROOT / "docs" / "REPO-HYGIENE.it.md",
            ".it.md",
            errors,
        )

        self.assertEqual(errors, [])

    def test_sources_policy_pair(self) -> None:
        errors: list[str] = []

        validator.validate_pair(
            ROOT / "sources" / "README.md",
            ROOT / "sources" / "README.it.md",
            ".it.md",
            errors,
        )

        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
