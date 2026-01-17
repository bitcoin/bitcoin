#!/usr/bin/env python3
"""Procesa ideas de agentes y guarda resultados en JSONL."""
from __future__ import annotations

import argparse
import json
from datetime import datetime
from pathlib import Path
from typing import Any, Iterable

IDEAS_FILE = Path(__file__).resolve().parent / "agents_ideas.jsonl"
OUTPUTS_FILE = Path(__file__).resolve().parent / "agents_outputs.jsonl"


def load_jsonl(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8") as handle:
        return [json.loads(line) for line in handle if line.strip()]


def append_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row, ensure_ascii=False) + "\n")


def process_ideas(ideas_file: Path = IDEAS_FILE, outputs_file: Path = OUTPUTS_FILE) -> int:
    ideas = load_jsonl(ideas_file)
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
    args = parser.parse_args()

    total = process_ideas(args.ideas_file, args.outputs_file)
    print(f"Procesadas {total} ideas.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
