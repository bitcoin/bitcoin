#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

from pathlib import Path
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
    print("Export only allowed settings:")
    # Hardcode the list of allowed settings instead of grepping scripts to prevent injection.
    # This list is derived from all variables exported in ci/test/00_setup_env*.sh
    settings = {
        "APT_LLVM_V",
        "BASE_BUILD_DIR",
        "BASE_OUTDIR",
        "BASE_READ_ONLY_DIR",
        "BASE_ROOT_DIR",
        "BASE_SCRATCH_DIR",
        "BITCOIN_CMD",
        "BITCOIN_CONFIG",
        "BITCOIN_CONFIG_ALL",
        "BOOST_TEST_RANDOM",
        "CCACHE_COMPRESS",
        "CCACHE_DIR",
        "CCACHE_MAXSIZE",
        "CCACHE_TEMPDIR",
        "CI_BASE_PACKAGES",
        "CI_CONTAINER_CAP",
        "CI_FAILFAST_TEST_LEAVE_DANGLING",
        "CI_IMAGE_NAME_TAG",
        "CI_IMAGE_PLATFORM",
        "CI_LIMIT_STACK_SIZE",
        "CI_OS_NAME",
        "CI_RETRY_EXE",
        "CMAKE_GENERATOR",
        "CONTAINER_NAME",
        "DEBIAN_FRONTEND",
        "DEPENDS_DIR",
        "DEP_OPTS",
        "DIR_QA_ASSETS",
        "DOWNLOAD_PREVIOUS_RELEASES",
        "DOCKER_BUILD_CACHE_ARG",
        "DPKG_ADD_ARCH",
        "FILE_ENV",
        "FUZZ_TESTS_CONFIG",
        "GOAL",
        "HOST",
        "LC_ALL",
        "MAKEJOBS",
        "MSAN_AND_LIBCXX_FLAGS",
        "MSAN_FLAGS",
        "NO_DEPENDS",
        "OSX_SDK",
        "PACKAGES",
        "PIP_PACKAGES",
        "PREVIOUS_RELEASES_DIR",
        "RUN_CHECK_DEPS",
        "RUN_FUNCTIONAL_TESTS",
        "RUN_FUZZ_TESTS",
        "RUN_IWYU",
        "RUN_TIDY",
        "RUN_UNIT_TESTS",
        "SDK_URL",
        "TEST_RUNNER_EXTRA",
        "TEST_RUNNER_TIMEOUT_FACTOR",
        "TIDY_LLVM_V",
        "USE_INSTRUMENTED_LIBCPP",
        "USE_VALGRIND",
        "XCODE_BUILD_ID",
        "XCODE_VERSION",
    }

    # Append $USER to /tmp/env to support multi-user systems and $CONTAINER_NAME
    # to allow support starting multiple runs simultaneously by the same user.
    # Sanitize inputs for file name safety.
    user_sanitized = "".join(c for c in os.environ.get("USER", "unknown") if c.isalnum())
    container_sanitized = "".join(c for c in os.environ["CONTAINER_NAME"] if c.isalnum())
    env_file = f"/tmp/env-{user_sanitized}-{container_sanitized}"
    with open(env_file, "w") as file:
        for k, v in os.environ.items():
            if k in settings:
                file.write(f"{k}={v}\n")
    run(["cat", env_file])

    if os.getenv("DANGER_RUN_CI_ON_HOST"):
        print("Running on host system without docker wrapper")
        print("Create missing folders")
        for create_dir in [
                os.environ["CCACHE_DIR"],
                os.environ["PREVIOUS_RELEASES_DIR"],
        ]:
            Path(create_dir).mkdir(parents=True, exist_ok=True)

        # Modify PATH to prepend the retry script, needed for CI_RETRY_EXE
        os.environ["PATH"] = f"{os.environ['BASE_ROOT_DIR']}/ci/retry:{os.environ['PATH']}"
        # GNU getopt is required for the CI_RETRY_EXE script
        if os.getenv("CI_OS_NAME") == "macos":
            prefix = run(
                ["brew", "--prefix", "gnu-getopt"],
                stdout=subprocess.PIPE,
                text=True,
            ).stdout.strip()
            os.environ["IN_GETOPT_BIN"] = f"{prefix}/bin/getopt"
    else:
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
        if run(cmd_build, check=False).returncode != 0:
            print(f"Retry building {os.environ['CONTAINER_NAME']} image tag after failure")
            time.sleep(3)
            run(cmd_build)

        for suffix in ["ccache", "depends", "depends_sources", "previous_releases"]:
            run(["docker", "volume", "create", f"{os.environ['CONTAINER_NAME']}_{suffix}"], check=False)

        CI_CCACHE_MOUNT = f"type=volume,src={os.environ['CONTAINER_NAME']}_ccache,dst={os.environ['CCACHE_DIR']}"
        CI_DEPENDS_MOUNT = f"type=volume,src={os.environ['CONTAINER_NAME']}_depends,dst={os.environ['DEPENDS_DIR']}/built"
        CI_DEPENDS_SOURCES_MOUNT = f"type=volume,src={os.environ['CONTAINER_NAME']}_depends_sources,dst={os.environ['DEPENDS_DIR']}/sources"
        CI_PREVIOUS_RELEASES_MOUNT = f"type=volume,src={os.environ['CONTAINER_NAME']}_previous_releases,dst={os.environ['PREVIOUS_RELEASES_DIR']}"
        CI_BUILD_MOUNT = []

        if os.getenv("DANGER_CI_ON_HOST_FOLDERS"):
            # ensure the directories exist
            for create_dir in [
                    os.environ["CCACHE_DIR"],
                    f"{os.environ['DEPENDS_DIR']}/built",
                    f"{os.environ['DEPENDS_DIR']}/sources",
                    os.environ["PREVIOUS_RELEASES_DIR"],
                    os.environ["BASE_BUILD_DIR"],  # Unset by default, must be defined externally
            ]:
                Path(create_dir).mkdir(parents=True, exist_ok=True)

            CI_CCACHE_MOUNT = f"type=bind,src={os.environ['CCACHE_DIR']},dst={os.environ['CCACHE_DIR']}"
            CI_DEPENDS_MOUNT = f"type=bind,src={os.environ['DEPENDS_DIR']}/built,dst={os.environ['DEPENDS_DIR']}/built"
            CI_DEPENDS_SOURCES_MOUNT = f"type=bind,src={os.environ['DEPENDS_DIR']}/sources,dst={os.environ['DEPENDS_DIR']}/sources"
            CI_PREVIOUS_RELEASES_MOUNT = f"type=bind,src={os.environ['PREVIOUS_RELEASES_DIR']},dst={os.environ['PREVIOUS_RELEASES_DIR']}"
            CI_BUILD_MOUNT = [f"--mount=type=bind,src={os.environ['BASE_BUILD_DIR']},dst={os.environ['BASE_BUILD_DIR']}"]

        if os.getenv("DANGER_CI_ON_HOST_CCACHE_FOLDER"):
            if not os.path.isdir(os.environ["CCACHE_DIR"]):
                print(f"Error: Directory '{os.environ['CCACHE_DIR']}' must be created in advance.")
                sys.exit(1)
            CI_CCACHE_MOUNT = f"type=bind,src={os.environ['CCACHE_DIR']},dst={os.environ['CCACHE_DIR']}"

        run(["docker", "network", "create", "--ipv6", "--subnet", "1111:1111::/112", "ci-ip6net"], check=False)

        if os.getenv("RESTART_CI_DOCKER_BEFORE_RUN"):
            print("Restart docker before run to stop and clear all containers started with --rm")
            run(["podman", "container", "rm", "--force", "--all"])  # Similar to "systemctl restart docker"

            # Still prune everything in case the filtered pruning doesn't work, or if labels were not set
            # on a previous run. Belt and suspenders approach, should be fine to remove in the future.
            # Prune images used by --external containers (e.g. build containers) when
            # using podman.
            print("Prune all dangling images")
            run(["podman", "image", "prune", "--force", "--external"])

        print(f"Prune all dangling {CI_IMAGE_LABEL} images")
        # When detecting podman-docker, `--external` should be added.
        run(["docker", "image", "prune", "--force", "--filter", f"label={CI_IMAGE_LABEL}"])

        cmd_run = ["docker", "run", "--rm", "--interactive", "--detach", "--tty"]
        cmd_run += [
            "--cap-add=LINUX_IMMUTABLE",
            *shlex.split(os.getenv("CI_CONTAINER_CAP", "")),
            f"--mount=type=bind,src={os.environ['BASE_READ_ONLY_DIR']},dst={os.environ['BASE_READ_ONLY_DIR']},readonly",
            f"--mount={CI_CCACHE_MOUNT}",
            f"--mount={CI_DEPENDS_MOUNT}",
            f"--mount={CI_DEPENDS_SOURCES_MOUNT}",
            f"--mount={CI_PREVIOUS_RELEASES_MOUNT}",
            *CI_BUILD_MOUNT,
            f"--env-file={env_file}",
            f"--name={os.environ['CONTAINER_NAME']}",
            "--network=ci-ip6net",
            f"--platform={os.environ['CI_IMAGE_PLATFORM']}",
            os.environ["CONTAINER_NAME"],
        ]

        container_id = run(
            cmd_run,
            stdout=subprocess.PIPE,
            text=True,
        ).stdout.strip()

    def ci_exec(cmd_inner, **kwargs):
        if os.getenv("DANGER_RUN_CI_ON_HOST"):
            prefix = []
        else:
            prefix = ["docker", "exec", container_id]

        return run([*prefix, *cmd_inner], **kwargs)

    # Normalize all folders to BASE_ROOT_DIR
    ci_exec([
        "rsync",
        "--recursive",
        "--perms",
        "--stats",
        "--human-readable",
        f"{os.environ['BASE_READ_ONLY_DIR']}/",
        f"{os.environ['BASE_ROOT_DIR']}",
    ])
    ci_exec([f"{os.environ['BASE_ROOT_DIR']}/ci/test/01_base_install.sh"])
    ci_exec([f"{os.environ['BASE_ROOT_DIR']}/ci/test/03_test_script.sh"])

    if not os.getenv("DANGER_RUN_CI_ON_HOST"):
        print("Stop and remove CI container by ID")
        run(["docker", "container", "kill", container_id])


if __name__ == "__main__":
    main()
