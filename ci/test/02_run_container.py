#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import os
import random
import shlex
import subprocess
import sys


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except Exception as e:
        sys.exit(e)


def maybe_cpuset():
    # Env vars during the build can not be changed. For example, a modified
    # $MAKEJOBS is ignored in the build process. Use --cpuset-cpus as an
    # approximation to respect $MAKEJOBS somewhat, if cpuset is available.
    if os.getenv("HAVE_CGROUP_CPUSET"):
        P = int(run(['nproc'], stdout=subprocess.PIPE).stdout)
        M = min(P, int(os.getenv("MAKEJOBS").lstrip("-j")))
        cpus = sorted(random.sample(range(P), M))
        return [f"--cpuset-cpus={','.join(map(str, cpus))}"]
    return []


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

        maybe_cache_arg = shlex.split(os.getenv('DOCKER_BUILD_CACHE_ARG'))

        # Use buildx unconditionally
        # Using buildx is required to properly load the correct driver, for use with registry caching. Neither build, nor BUILDKIT=1 currently do this properly
        cmd_build = ["docker", "buildx", "build"]
        cmd_build += [
            f"--file={os.getenv('BASE_READ_ONLY_DIR')}/ci/test_imagefile",
            f"--build-arg=CI_IMAGE_NAME_TAG={os.getenv('CI_IMAGE_NAME_TAG')}",
            f"--build-arg=FILE_ENV={os.getenv('FILE_ENV')}",
            f"--build-arg=BASE_ROOT_DIR={os.getenv('BASE_ROOT_DIR')}",
            f"--platform={os.getenv('CI_IMAGE_PLATFORM')}",
            f"--label={CI_IMAGE_LABEL}",
            f"--tag={os.getenv('CONTAINER_NAME')}",
        ]
        cmd_build += maybe_cpuset()
        cmd_build += maybe_cache_arg
        cmd_build += [os.getenv('BASE_READ_ONLY_DIR')]

        print(f"Building {os.getenv('CI_IMAGE_NAME_TAG')} image to run in")
        run(cmd_build)

    run(["./ci/test/02_run_container.sh"])  # run the remainder


if __name__ == "__main__":
    main()
