#!/usr/bin/env python3
"""Genera un reporte del directorio contrib/satoshi-vission."""
from __future__ import annotations

import argparse
from datetime import datetime
from pathlib import Path


def format_entry(path: Path, root: Path) -> str:
    rel_path = path.relative_to(root)
    stat = path.stat()
    size = stat.st_size
    modified = datetime.fromtimestamp(stat.st_mtime).isoformat(timespec="seconds")
    return f"- {rel_path} ({size} bytes, mtime: {modified})"


def build_report(root: Path) -> list[str]:
    lines = ["# Reporte Satoshi Vission", ""]
    lines.append(f"Raíz: {root}")
    lines.append("")
    lines.append("## Entradas")
    for path in sorted(root.rglob("*")):
        lines.append(format_entry(path, root))
    return lines


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Genera un reporte del árbol contrib/satoshi-vission",
    )
    parser.add_argument(
        "--root",
        default=Path(__file__).resolve().parents[1],
        type=Path,
        help="Ruta base de contrib/satoshi-vission",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Ruta para guardar el reporte (por defecto, stdout)",
    )
    args = parser.parse_args()

    report_lines = build_report(args.root)
    report = "\n".join(report_lines)

    if args.output:
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
