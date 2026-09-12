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
from datetime import datetime, timedelta, timezone
from pathlib import Path

MAX_RUNTIME = timedelta(hours=5, minutes=30)
MIN_CHECK_AGE = timedelta(days=1)
SILENT_CHECK_NAME = "Periodic silent merge check"
RESULTS_FILE = Path("silent-merge-results.json")


def log(msg):
    print(msg, flush=True)


def starts_with_timestamp(line):
    first = line.split(maxsplit=1)[0] if line.strip() else ""
    try:
        datetime.fromisoformat(first.replace("Z", "+00:00"))
        return True
    except ValueError:
        return False


def run_capture_timestamped(cmd, **kwargs):
    """Run cmd, capturing its combined output with a timestamp on every line.

    Lines that already start with a timestamp (e.g. functional test framework or
    bitcoind output) are left untouched to avoid double-stamping.
    """
    log("+ " + shlex.join(cmd))
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, bufsize=1, **kwargs)
    lines = []
    for line in proc.stdout:
        line = line.rstrip("\n")
        if not starts_with_timestamp(line):
            timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            line = f"{timestamp} {line}"
        lines.append(line)
    proc.wait()
    return proc.returncode, "\n".join(lines)


def fmt_duration(td):
    total = int(td.total_seconds())
    d, rem = divmod(total, 86400)
    h, rem = divmod(rem, 3600)
    m = rem // 60
    if d:
        return f"{d}d {h}h {m}m" if m else f"{d}d {h}h"
    return f"{h}h {m}m" if m else f"{h}h"


def run(cmd, **kwargs):
    log("+ " + shlex.join(cmd))
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


def latest_passing_check_time(check_runs):
    times = []
    for check in check_runs:
        if check.get("name") == SILENT_CHECK_NAME:
            continue
        if check.get("conclusion") == "success":
            timestamp = check.get("completed_at")
            if timestamp:
                times.append(datetime.fromisoformat(timestamp))
    return max(times) if times else None


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
            log(f"PR #{pr_number} mergeability not yet computed, retrying in {delay}s...")
            time.sleep(delay)
    log(f"PR #{pr_number} mergeability still unknown after {retries} attempts, assuming mergeable.")
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

    run(["git", "config", "user.email", "action@github.com"])
    run(["git", "config", "user.name", "GitHub Action"])

    max_runtime_seconds = int(MAX_RUNTIME.total_seconds())
    start_time = time.monotonic()

    log("Fetching open PRs...")
    open_prs = get_open_prs(repo)
    log(f"Found {len(open_prs)} open PRs")

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
        log(f"{len(candidate_prs)} candidate PR(s) found")
        log("Processing PRs that have never been checked first and then by oldest check")

    for pr in candidate_prs:
        if time.monotonic() - start_time >= max_runtime_seconds:
            log(f"Reached maximum runtime ({fmt_duration(MAX_RUNTIME)}); stopping before next PR.")
            break
        log(f"Checking PR #{pr['number']}")

        if pr["latest_silent_check"] is not None:
            time_since = datetime.now(tz=pr["latest_silent_check"].tzinfo) - pr["latest_silent_check"]
            log(f"PR #{pr['number']} was last checked {fmt_duration(time_since)} ago")
            print(f"::notice title=Recheck interval PR #{pr['number']}::Last checked {fmt_duration(time_since)} ago")

        if not get_pr_mergeable(repo, pr["number"]):
            log(f"PR #{pr['number']} has merge conflicts, skipping.")
            continue

        if has_failing_checks(pr["check_runs"]):
            log(f"PR #{pr['number']} already has failing check runs, skipping.")
            continue

        latest_pass = latest_passing_check_time(pr["check_runs"])
        if latest_pass is None:
            log(f"PR #{pr['number']} has no passing check runs, skipping.")
            continue
        age = datetime.now(tz=latest_pass.tzinfo) - latest_pass
        if age < MIN_CHECK_AGE:
            log(f"PR #{pr['number']} latest passing check is only {fmt_duration(age)} old (minimum {fmt_duration(MIN_CHECK_AGE)}), skipping.")
            continue

        # Merge the PR head against current master locally. GitHub's pull/{n}/merge ref can be
        # stale, so we cannot rely on it to test against the latest master.
        run(["git", "fetch", "origin", f"pull/{pr['number']}/head:pr-{pr['number']}-head"])
        run(["git", "checkout", "-B", "test-merge", "origin/master"])
        merge_result = run(["git", "merge", "--no-edit", f"pr-{pr['number']}-head"], check=False)
        if merge_result.returncode != 0:
            run(["git", "merge", "--abort"], check=False)
            log(f"PR #{pr['number']} has merge conflicts against current master, skipping.")
            continue

        ci_returncode, ci_output = run_capture_timestamped([".github/ci-test-each-commit-exec.py"])
        if ci_returncode != 0:
            log(f"PR #{pr['number']} CI script failed. Output follows (expand group):")
            print(f"::group::CI output for PR #{pr['number']} (failed)")
            print(ci_output, flush=True)
            print("::endgroup::", flush=True)
            conclusion = "failure"
            summary = (
                f"CMake build or tests failed when this PR was merged with current master. "
                f"To find the failure, open the linked workflow run and expand the collapsed log "
                f"group 'CI output for PR #{pr['number']} (failed)'. Please rebase and ensure all checks pass."
            )
        else:
            log(f"PR #{pr['number']} CI completed successfully.")
            conclusion = "success"
            summary = "CMake build and tests passed."

        append_result(
            pr_number=pr["number"],
            head_sha=pr["head_sha"],
            conclusion=conclusion,
            summary=summary,
        )

        stats = run(["ccache", "--show-stats"], stdout=subprocess.PIPE, check=False)
        if stats.returncode == 0:
            stats_inline = stats.stdout.strip().replace("\n", "%0A")
            print(f"::notice title=ccache stats PR #{pr['number']}::{stats_inline}")


if __name__ == "__main__":
    main()
