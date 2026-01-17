#!/usr/bin/env python3
"""Procesa ideas de agentes y guarda resultados en JSONL."""
from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
from datetime import datetime
from pathlib import Path
from typing import Any, Iterable

IDEAS_FILE = Path(__file__).resolve().parent / "agents_ideas.jsonl"
OUTPUTS_FILE = Path(__file__).resolve().parent / "agents_outputs.jsonl"


def load_jsonl(path: Path, *, strict: bool = False) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8") as handle:
        entries: list[dict[str, Any]] = []
        for line_number, line in enumerate(handle, start=1):
            if not line.strip():
                continue
            try:
                entries.append(json.loads(line))
            except json.JSONDecodeError as exc:
                print(
                    f"Warning: failed to decode JSON in {path} at line {line_number}: {exc}",
                    file=sys.stderr,
                )
                if strict:
                    raise
        return entries


def append_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    existing_content = ""
    original_mode = None
    if path.exists():
        existing_content = path.read_text(encoding="utf-8")
        original_mode = path.stat().st_mode

    temp_path: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            "w",
            encoding="utf-8",
            dir=path.parent,
            delete=False,
        ) as handle:
            temp_path = handle.name
            if existing_content:
                handle.write(existing_content)
            for row in rows:
                handle.write(json.dumps(row, ensure_ascii=False) + "\n")
        if original_mode is not None:
            os.chmod(temp_path, original_mode)
        os.replace(temp_path, path)
    finally:
        if temp_path is not None and os.path.exists(temp_path):
            os.unlink(temp_path)


def process_ideas(
    ideas_file: Path = IDEAS_FILE,
    outputs_file: Path = OUTPUTS_FILE,
    *,
    strict: bool = False,
) -> int:
    ideas = load_jsonl(ideas_file, strict=strict)
    if not ideas:
        return 0

    processed_at = datetime.utcnow().isoformat(timespec="seconds") + "Z"
    outputs = [
        {"idea": idea, "processed_at": processed_at}
        for idea in ideas
    ]
    append_jsonl(outputs_file, outputs)
    return len(outputs)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--ideas-file",
        type=Path,
        default=IDEAS_FILE,
        help="Ruta al archivo JSONL con ideas a procesar.",
    )
    parser.add_argument(
        "--outputs-file",
        type=Path,
        default=OUTPUTS_FILE,
        help="Ruta al archivo JSONL para guardar los resultados.",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Falla al primer registro JSONL malformado.",
    )
    args = parser.parse_args()

    total = process_ideas(args.ideas_file, args.outputs_file, strict=args.strict)
    print(f"Procesadas {total} ideas.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
