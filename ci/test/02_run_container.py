#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import os
import shlex
import subprocess
import sys


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except Exception as e:
        sys.exit(e)


def main():
    print("Export only allowed settings:")
    settings = run(
        ["bash", "-c", "grep export ./ci/test/00_setup_env*.sh"],
        stdout=subprocess.PIPE,
        text=True,
        encoding="utf8",
    ).stdout.splitlines()
    settings = set(l.split("=")[0].split("export ")[1] for l in settings)
    # Add "hidden" settings, which are never exported, manually. Otherwise,
    # they will not be passed on.
    settings.update([
        "BASE_BUILD_DIR",
        "CI_FAILFAST_TEST_LEAVE_DANGLING",
    ])

    # Append $USER to /tmp/env to support multi-user systems and $CONTAINER_NAME
    # to allow support starting multiple runs simultaneously by the same user.
    env_file = "/tmp/env-{u}-{c}".format(
        u=os.getenv("USER"),
        c=os.getenv("CONTAINER_NAME"),
    )
    with open(env_file, "w", encoding="utf8") as file:
        for k, v in os.environ.items():
            if k in settings:
                file.write(f"{k}={v}\n")
    run(["cat", env_file])

    if not os.getenv("DANGER_RUN_CI_ON_HOST"):
        CI_IMAGE_LABEL = "bitcoin-ci-test"

        # Use buildx unconditionally
        # Using buildx is required to properly load the correct driver, for use with registry caching. Neither build, nor BUILDKIT=1 currently do this properly
        cmd_build = ["docker", "buildx", "build"]
        cmd_build += [
            f"--file={os.environ['BASE_READ_ONLY_DIR']}/ci/test_imagefile",
            f"--build-arg=CI_IMAGE_NAME_TAG={os.environ['CI_IMAGE_NAME_TAG']}",
            f"--build-arg=FILE_ENV={os.environ['FILE_ENV']}",
            f"--build-arg=BASE_ROOT_DIR={os.environ['BASE_ROOT_DIR']}",
            f"--platform={os.environ['CI_IMAGE_PLATFORM']}",
            f"--label={CI_IMAGE_LABEL}",
            f"--tag={os.environ['CONTAINER_NAME']}",
        ]
        cmd_build += shlex.split(os.getenv("DOCKER_BUILD_CACHE_ARG", ""))
        cmd_build += [os.environ["BASE_READ_ONLY_DIR"]]

        print(f"Building {os.environ['CONTAINER_NAME']} image tag to run in")
        run(cmd_build)

    run(["./ci/test/02_run_container.sh"])  # run the remainder


if __name__ == "__main__":
    main()
