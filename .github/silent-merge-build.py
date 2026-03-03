#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import json
import os
import shlex
import subprocess
import sys
import time
from datetime import datetime, timedelta
from pathlib import Path

MAX_RUNTIME = timedelta(hours=5, minutes=30)
SILENT_CHECK_NAME = "Periodic silent merge check"
RESULTS_FILE = Path("silent-merge-results.json")


def fmt_duration(td):
    total = int(td.total_seconds())
    h, rem = divmod(total, 3600)
    m = rem // 60
    return f"{h}h{m}m" if m else f"{h}h"


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    kwargs.setdefault("text", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(str(e))


def gh_api_paginated(url, key=None):
    result = run(["gh", "api", "--paginate", url], stdout=subprocess.PIPE)
    items = []
    for page in result.stdout.strip().splitlines():
        data = json.loads(page)
        items.extend(data.get(key) if key else data)
    return items


def get_open_prs(repo):
    return gh_api_paginated(f"repos/{repo}/pulls?state=open&draft=false&per_page=100")


def get_check_runs(repo, head_sha):
    return gh_api_paginated(f"repos/{repo}/commits/{head_sha}/check-runs", key="check_runs")


def has_failing_checks(check_runs):
    return any(
        check.get("conclusion") == "failure" and check.get("name") != SILENT_CHECK_NAME
        for check in check_runs
    )


def append_result(pr_number, head_sha, conclusion, summary):
    results = json.loads(RESULTS_FILE.read_text()) if RESULTS_FILE.exists() else []
    results.append({"pr_number": pr_number, "head_sha": head_sha, "conclusion": conclusion, "summary": summary})
    RESULTS_FILE.write_text(json.dumps(results, indent=2))


def get_pr_mergeable(repo, pr_number, retries=5, delay=5):
    """Fetch a PR individually and poll until GitHub has computed its mergeability."""
    # https://docs.github.com/en/rest/guides/using-the-rest-api-to-interact-with-your-git-database?apiVersion=2026-03-10#checking-mergeability-of-pull-requests
    for attempt in range(retries):
        result = run(["gh", "api", f"repos/{repo}/pulls/{pr_number}"], stdout=subprocess.PIPE)
        mergeable = json.loads(result.stdout).get("mergeable")
        if mergeable is not None:
            return mergeable
        if attempt < retries - 1:
            print(f"PR #{pr_number} mergeability not yet computed, retrying in {delay}s...")
            time.sleep(delay)
    print(f"PR #{pr_number} mergeability still unknown after {retries} attempts, assuming mergeable.")
    return True


def latest_check_time(check_runs):
    times = []
    for check in check_runs:
        if check.get("name") != SILENT_CHECK_NAME:
            continue
        timestamp = check.get("completed_at") or check.get("started_at")
        if timestamp:
            times.append(datetime.fromisoformat(timestamp))
    return max(times) if times else None


def main():
    repo = os.environ.get("GITHUB_REPOSITORY")
    if not repo:
        sys.exit("GITHUB_REPOSITORY is not set")

    repo_root = Path(__file__).resolve().parent.parent
    os.chdir(repo_root)

    max_runtime_seconds = int(MAX_RUNTIME.total_seconds())
    start_time = time.monotonic()

    print("Fetching open PRs...")
    open_prs = get_open_prs(repo)
    print(f"Found {len(open_prs)} open PRs")

    prs = []
    for pr_json in open_prs:
        pr_number = pr_json.get("number")
        head_sha = pr_json.get("head", {}).get("sha")
        check_runs = get_check_runs(repo, head_sha) if head_sha else []
        prs.append(
            {
                "number": pr_number,
                "head_sha": head_sha,
                "created_at": pr_json.get("created_at"),
                "check_runs": check_runs,
                "latest_silent_check": latest_check_time(check_runs),
            }
        )

    candidate_prs = sorted(
        [pr for pr in prs if pr["latest_silent_check"] is None],
        key=lambda pr: pr["created_at"] or "")

    candidate_prs.extend(sorted(
            [pr for pr in prs if pr["latest_silent_check"] is not None],
            key=lambda pr: pr["latest_silent_check"]))

    if candidate_prs:
        print(f"{len(candidate_prs)} candidate PR(s) found")
        print("Processing PRs that have never been checked first and then by oldest check")

    for pr in candidate_prs:
        if time.monotonic() - start_time >= max_runtime_seconds:
            print(f"Reached maximum runtime ({fmt_duration(MAX_RUNTIME)}); stopping before next PR.")
            break
        print(f"Checking PR #{pr['number']}")

        if not get_pr_mergeable(repo, pr["number"]):
            print(f"PR #{pr['number']} has merge conflicts, skipping.")
            continue

        if has_failing_checks(pr["check_runs"]):
            print(f"PR #{pr['number']} already has failing check runs, skipping.")
            continue

        run(["git", "fetch", "origin", f"pull/{pr['number']}/merge:pr-{pr['number']}"])
        run(["git", "checkout", f"pr-{pr['number']}"])

        ci_result = run([".github/ci-test-each-commit-exec.py"], check=False)
        if ci_result.returncode != 0:
            print(f"PR #{pr['number']} CI script failed.")
            conclusion = "failure"
            summary = "CMake build or tests failed."
        else:
            conclusion = "success"
            summary = "CMake build and tests passed."

        append_result(
            pr_number=pr["number"],
            head_sha=pr["head_sha"],
            conclusion=conclusion,
            summary=summary,
        )


if __name__ == "__main__":
    main()
