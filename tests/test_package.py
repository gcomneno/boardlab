"""Smoke tests for the installable BoardLab package."""

from importlib import import_module


def test_package_is_importable() -> None:
    """The project package must be importable through the installed environment."""
    module = import_module("boardlab")

    assert module.__name__ == "boardlab"
