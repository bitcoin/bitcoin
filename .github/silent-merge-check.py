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


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    kwargs.setdefault("text", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(str(e))


def gh_api(args, **kwargs):
    return run(["gh", "api", *args], **kwargs)


def list_open_prs(repo):
    result = run(
        ["gh", "pr", "list", "--state", "open", "--json", "number", "--repo", repo],
        stdout=subprocess.PIPE,
    )
    return [item["number"] for item in json.loads(result.stdout or "[]")]


def get_pr(repo, pr_number):
    result = gh_api([f"repos/{repo}/pulls/{pr_number}"], stdout=subprocess.PIPE)
    return json.loads(result.stdout)


def get_check_runs(repo, head_sha):
    result = gh_api(
        [f"repos/{repo}/commits/{head_sha}/check-runs"],
        stdout=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        return []
    return json.loads(result.stdout or "{}").get("check_runs", [])


def get_failed_checks(check_runs, excluded_check):
    return [
        check.get("name")
        for check in check_runs
        if check.get("conclusion") == "failure" and check.get("name") != excluded_check
    ]


def create_check_run(repo, name, head_sha, conclusion, title, summary):
    run(
        [
            "gh",
            "api",
            f"repos/{repo}/check-runs",
            "-X",
            "POST",
            "-F",
            f"name={name}",
            "-F",
            f"head_sha={head_sha}",
            "-F",
            "status=completed",
            "-F",
            f"conclusion={conclusion}",
            "-F",
            f"output[title]={title}",
            "-F",
            f"output[summary]={summary}",
        ]
    )


def parse_timestamp(value):
    if not value:
        return None
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None


def latest_check_time(check_runs, check_name):
    times = []
    for check in check_runs:
        if check.get("name") != check_name:
            continue
        timestamp = check.get("completed_at") or check.get("started_at")
        parsed = parse_timestamp(timestamp)
        if parsed:
            times.append(parsed)
    return max(times) if times else None


def main():
    repo = os.environ.get("GITHUB_REPOSITORY")
    if not repo:
        sys.exit("GITHUB_REPOSITORY is not set")

    repo_root = Path(__file__).resolve().parent.parent
    os.chdir(repo_root)
    start_time = time.monotonic()
    max_runtime_seconds = int(timedelta(hours=5, minutes=30).total_seconds())
    silent_check_name = "Periodic silent merge check"

    print("Fetching open PRs...")
    prs = list_open_prs(repo)
    print(f"Found PRs: {':'.join(str(pr) for pr in prs)}")

    pr_infos = []
    for pr in prs:
        pr_json = get_pr(repo, pr)
        head_sha = pr_json.get("head", {}).get("sha")
        check_runs = get_check_runs(repo, head_sha) if head_sha else []
        pr_infos.append(
            {
                "number": pr,
                "draft": pr_json.get("draft"),
                "mergeable_state": pr_json.get("mergeable_state"),
                "head_sha": head_sha,
                "check_runs": check_runs,
                "latest_silent_check": latest_check_time(check_runs, silent_check_name),
            }
        )

    eligible_prs = [info for info in pr_infos if not info["draft"]]
    missing_checks = [info for info in eligible_prs if info["latest_silent_check"] is None]
    if missing_checks:
        candidates = missing_checks
        print(
            f"{len(candidates)} PR(s) missing {silent_check_name}; processing those first."
        )
    else:
        candidates = sorted(
            eligible_prs,
            key=lambda info: info["latest_silent_check"]
            or datetime.min.replace(tzinfo=timezone.utc),
        )
        if candidates:
            print(
                f"All PRs have {silent_check_name}; processing oldest check run from PR #{candidates[0]['number']}."
            )
            candidates = [candidates[0]]

    for info in candidates:
        if time.monotonic() - start_time >= max_runtime_seconds:
            print("Reached maximum runtime (5h30m); stopping before next PR.")
            break
        pr = info["number"]
        print(f"Checking PR #{pr}")

        if info["draft"] is True:
            print(f"PR #{pr} is draft, skipping.")
            continue

        if info["mergeable_state"] == "dirty":
            print(f"PR #{pr} has merge conflicts, reporting failed check.")
            create_check_run(
                repo,
                name="Conflict Check",
                head_sha=info["head_sha"],
                conclusion="failure",
                title="Merge conflicts detected",
                summary="This PR cannot be merged due to conflicts.",
            )
            continue

        excluded_check = silent_check_name
        failed_checks = get_failed_checks(info["check_runs"], excluded_check)
        if failed_checks:
            print(
                f"PR #{pr} already has failing check runs: {', '.join(failed_checks)}. Skipping."
            )
            continue

        if not info["head_sha"]:
            print(f"PR #{pr} has no head SHA, skipping.")
            continue

        run(["git", "fetch", "origin", f"pull/{pr}/merge:pr-{pr}"])
        run(["git", "checkout", f"pr-{pr}"])

        ci_result = run(["./ci/test_run_all.sh"], check=False)
        if ci_result.returncode != 0:
            print(f"PR #{pr} CI script failed.")
            conclusion = "failure"
            summary = "CMake build or tests failed."
        else:
            conclusion = "success"
            summary = "CMake build and tests passed."

        create_check_run(
            repo,
            name=silent_check_name,
            head_sha=info["head_sha"],
            conclusion=conclusion,
            title=silent_check_name,
            summary=summary,
        )


if __name__ == "__main__":
    main()
