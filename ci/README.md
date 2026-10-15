# CI Scripts

This directory contains scripts for each build step in each build stage.

## Running a Stage Locally

Be aware that the tests will be built and run in-place, so please run at your own risk.
If the repository is not a fresh git clone, you might have to clean files from previous builds or test runs first.

The ci needs to perform various sysadmin tasks such as installing packages or writing to the user's home directory.
While it should be fine to run
the ci system locally on your development box, the ci scripts can generally be assumed to have received less review and
testing compared to other parts of the codebase. If you want to keep the work tree clean, you might want to run the ci
system in a virtual machine with a Linux operating system of your choice.

To allow for a wide range of tested environments, but also ensure reproducibility to some extent, the test stage
requires `bash`, `docker`, and `python3` to be installed. On Ubuntu, you may
use the `docker.io` or the `podman-docker` package.

For some sanitizer builds, the kernel's address-space layout randomization
(ASLR) entropy can cause sanitizer shadow memory mappings to fail. When running
the CI locally you may need to reduce that entropy by running:

```
sudo sysctl -w vm.mmap_rnd_bits=28
```

To run a test that requires emulating a CPU architecture different from the
host, the CI relies on the container environment recognizing foreign executables
and automatically running them using QEMU. Refer to the container engine docs
to find out whether this is supported by default and whether a separate install
of `qemu-user-static` is needed
([Docker](https://docs.docker.com/build/building/multi-platform/#qemu)).
If not, the following may work, also for `podman`:

```
docker run --privileged --rm tonistiigi/binfmt --install all
```

It is recommended to run the CI system in a clean environment. The `env -i`
command below ensures that *only* specified environment variables are propagated
into the local CI.
To run the test stage with a specific configuration:

```
env -i HOME="$HOME" PATH="$PATH" USER="$USER" FILE_ENV="./ci/test/00_setup_env_arm.sh" ./ci/test_run_all.sh
```

## Configurations

The test files (`FILE_ENV`) are constructed to test a wide range of
configurations, rather than a single pass/fail. This helps to catch build
failures and logic errors that present on platforms other than the ones the
author has tested.

Some builders use the dependency-generator in `./depends`, rather than using
the system package manager to install build dependencies. This guarantees that
the tester is using the same versions as the release builds, which also use
`./depends`.

It is also possible to force a specific configuration without modifying the
file. For example,

```
env -i HOME="$HOME" PATH="$PATH" USER="$USER" MAKEJOBS="-j1" FILE_ENV="./ci/test/00_setup_env_arm.sh" ./ci/test_run_all.sh
```

## Cache

The CI system will cache container images to avoid having to reinstall all
packages on each run. For faster incremental builds, there are also container
volumes to hold the results of the dependency-generator from `./depends` and
to hold `ccache` results.

## Pinning dependencies

For direct downloads, checksums are generally used to protect against transport
errors.

Other CI dependencies are not generally pinned by hash. This is intentional,
because pins can delay normal and security updates until the next bump, and
require recurring manual review and maintenance. Moreover, a hash pin only
verifies the directly fetched artifact. It does not constrain recursive
downloads at runtime, and it could create a false sense of reproducibility and
security.

## Configuring a repository for hosted CI

### Primary repository

To configure the primary repository, follow these steps:

1. Register with [WarpBuild](https://www.warpbuild.com/) and purchase runners.
2. Install the WarpBuild GitHub app against the GitHub organization.
3. Enable organisation-level runners to be used in public repositories:
   1. `Org settings -> Actions -> Runner Groups -> Default -> Allow public repos`
4. Permit the following actions to run:
   1. actions/cache/restore@\*
   1. actions/cache/save@\*
   1. actions/github-script@\*
   1. docker/setup-buildx-action@\*
   1. warpbuilds/cache/restore@\*
   1. warpbuilds/cache/save@\*

### Forked repositories

When used in a fork the CI will run on GitHub's free hosted runners by default.
In this case, GitHub's cache size limitations may cause caches to be frequently evicted and missed, but the workflows will run (slowly).

It is also possible to use your own WarpBuild Runners in your own fork by replacing the references to `bitcoin/bitcoin` in `../.github/workflows/ci.yml` with the name of your fork (e.g. `your-org/bitcoin`).
NB that WarpBuild Runners only work at an organisation level, therefore in order to use your own WarpBuild Runners, *the fork must be within your own organisation*.
