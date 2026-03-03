#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

SILENT_CHECK_NAME = "Periodic silent merge check"
RESULTS_FILE = Path("silent-merge-results.json")


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    kwargs.setdefault("text", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(str(e))


def create_check_run(repo, head_sha, conclusion, summary):
    fields = {
        "name": SILENT_CHECK_NAME,
        "head_sha": head_sha,
        "status": "completed",
        "conclusion": conclusion,
        "output[title]": SILENT_CHECK_NAME,
        "output[summary]": summary,
    }
    field_args = [arg for k, v in fields.items() for arg in ("-F", f"{k}={v}")]
    run(["gh", "api", f"repos/{repo}/check-runs", "-X", "POST", *field_args])


def main():
    repo = os.environ.get("GITHUB_REPOSITORY")
    if not repo:
        sys.exit("GITHUB_REPOSITORY is not set")

    if not RESULTS_FILE.exists():
        print("No results file found, nothing to report.")
        return

    results = json.loads(RESULTS_FILE.read_text())
    print(f"Reporting {len(results)} result(s)...")

    for result in results:
        pr_number = result["pr_number"]
        print(f"Posting check run for PR #{pr_number} ({result['conclusion']})")
        create_check_run(
            repo,
            head_sha=result["head_sha"],
            conclusion=result["conclusion"],
            summary=result["summary"],
        )


if __name__ == "__main__":
    main()
