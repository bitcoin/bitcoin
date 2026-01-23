#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import os
import shlex
import subprocess
import sys
import time
from pathlib import Path


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(str(e))


def get_worktree_mounts(repo_root):
    git_path = repo_root / ".git"
    if not git_path.is_file():
        return []
    content = git_path.read_text().strip()
    if not content.startswith("gitdir: "):
        return []
    gitdir = (repo_root / content.removeprefix("gitdir: ")).resolve()
    main_gitdir = gitdir.parent.parent
    return [
        f"--volume={gitdir}:{gitdir}",
        f"--volume={main_gitdir}:{main_gitdir}:ro",
    ]


def main():
    repo_root = Path(__file__).resolve().parent.parent
    is_ci = os.environ.get("GITHUB_ACTIONS") == "true"
    container = "bitcoin-linter"

    build_cmd = [
        "docker",
        "buildx",
        "build",
        f"--tag={container}",
        *shlex.split(os.environ.get("DOCKER_BUILD_CACHE_ARG", "")),
        f"--file={repo_root}/ci/lint_imagefile",
        str(repo_root),
    ]
    if run(build_cmd, check=False).returncode != 0:
        if is_ci:
            print("Retry building image after failure")
            time.sleep(3)
        run(build_cmd)

    extra_env = []
    if is_ci:
        if os.environ.get("GITHUB_EVENT_NAME") == "pull_request":
            extra_env = ["--env", "LINT_CI_IS_PR=1"]
        elif os.environ.get("GITHUB_REPOSITORY") == "bitcoin/bitcoin":
            extra_env = ["--env", "LINT_CI_SANITY_CHECK_COMMIT_SIG=1"]

    run(
        [
            "docker",
            "run",
            "--rm",
            *extra_env,
            f"--volume={repo_root}:/bitcoin",
            *get_worktree_mounts(repo_root),
            *([] if is_ci else ["-it"]),
            container,
            *sys.argv[1:],
        ]
    )


if __name__ == "__main__":
    main()
