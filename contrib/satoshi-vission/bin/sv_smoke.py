#!/usr/bin/env python3
"""Smoke checks for Satoshi Vission experimental tooling."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys


REQUIRED_DIRS = ["agents", "mirror", "docs", "bin"]
REQUIRED_FILES = [
    "README.md",
    "docs/PROPOSAL_DRAFT.md",
]


def run_checks(root: Path) -> list[str]:
    errors: list[str] = []
    for rel_dir in REQUIRED_DIRS:
        path = root / rel_dir
        if not path.is_dir():
            errors.append(f"Falta directorio requerido: {rel_dir}")
    for rel_file in REQUIRED_FILES:
        path = root / rel_file
        if not path.is_file():
            errors.append(f"Falta archivo requerido: {rel_file}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Ejecuta pruebas mínimas para contrib/satoshi-vission",
    )
    parser.add_argument(
        "--root",
        default=Path(__file__).resolve().parents[1],
        type=Path,
        help="Ruta base de contrib/satoshi-vission",
    )
    args = parser.parse_args()

    root = args.root
    errors = run_checks(root)
    if errors:
        for error in errors:
            print(f"[FAIL] {error}")
        return 1

    print("[OK] Todas las comprobaciones mínimas pasaron.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
