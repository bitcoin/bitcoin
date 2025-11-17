#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

import os
import shlex
import subprocess
import sys
import time


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(e)


def main():
    CONTAINER_NAME = os.environ["CONTAINER_NAME"]

    build_cmd = [
        "docker", "buildx", "build",
        f"--tag={CONTAINER_NAME}",
        *shlex.split(os.getenv("DOCKER_BUILD_CACHE_ARG", "")),
        "--file=./ci/lint_imagefile",
        "."
    ]

    if run(build_cmd, check=False).returncode != 0:
        print("Retry building image tag after failure")
        time.sleep(3)
        run(build_cmd)

    extra_env = []
    if os.environ["GITHUB_EVENT_NAME"] == "pull_request":
        extra_env = ["--env", "LINT_CI_IS_PR=1"]
    if os.environ["GITHUB_EVENT_NAME"] != "pull_request" and os.environ["GITHUB_REPOSITORY"] == "bitcoin/bitcoin":
        extra_env = ["--env", "LINT_CI_SANITY_CHECK_COMMIT_SIG=1"]

    run([
        "docker",
        "run",
        "--rm",
        *extra_env,
        f"--volume={os.getcwd()}:/bitcoin",
        CONTAINER_NAME,
    ])


if __name__ == "__main__":
    main()
