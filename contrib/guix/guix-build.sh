#!/usr/bin/env bash
export LC_ALL=C
set -e -o pipefail

# Determine the maximum number of jobs to run simultaneously (overridable by
# environment)
MAX_JOBS="${MAX_JOBS:-$(nproc)}"

# Download the depends sources now as we won't have internet access in the build
# container
make -C "${PWD}/depends" -j"$MAX_JOBS" download ${V:+V=1} ${SOURCES_PATH:+SOURCES_PATH="$SOURCES_PATH"}

# Determine the reference time used for determinism (overridable by environment)
SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-$(git log --format=%at -1)}"

# Execute "$@" in a pinned, possibly older version of Guix, for reproducibility
# across time.
time-machine() {
    guix time-machine --url=https://github.com/dongcarl/guix.git \
                      --commit=b066c25026f21fb57677aa34692a5034338e7ee3 \
                      -- "$@"
}

# Function to be called when building for host ${1} and the user interrupts the
# build
int_trap() {
cat << EOF
** INT received while building ${1}, you may want to clean up the relevant
   output, deploy, and distsrc-* directories before rebuilding

Hint: To blow everything away, you may want to use:

  $ git clean -xdff --exclude='/depends/SDKs/*'

Specifically, this will remove all files without an entry in the index,
excluding the SDK directory. Practically speaking, this means that all ignored
and untracked files and directories will be wiped, allowing you to start anew.
EOF
}

# Deterministically build Bitcoin Core for HOSTs (overridable by environment)
# shellcheck disable=SC2153
for host in ${HOSTS=x86_64-linux-gnu arm-linux-gnueabihf aarch64-linux-gnu riscv64-linux-gnu x86_64-w64-mingw32}; do

    # Display proper warning when the user interrupts the build
    trap 'int_trap ${host}' INT

    (
        # Required for 'contrib/guix/manifest.scm' to output the right manifest
        # for the particular $HOST we're building for
        export HOST="$host"

        # Run the build script 'contrib/guix/libexec/build.sh' in the build
        # container specified by 'contrib/guix/manifest.scm'.
        #
        # Explanation of `guix environment` flags:
        #
        #   --container        run command within an isolated container
        #
        #     Running in an isolated container minimizes build-time differences
        #     between machines and improves reproducibility
        #
        #   --pure             unset existing environment variables
        #
        #     Same rationale as --container
        #
        #   --no-cwd           do not share current working directory with an
        #                      isolated container
        #
        #     When --container is specified, the default behavior is to share
        #     the current working directory with the isolated container at the
        #     same exact path (e.g. mapping '/home/satoshi/bitcoin/' to
        #     '/home/satoshi/bitcoin/'). This means that the $PWD inside the
        #     container becomes a source of irreproducibility. --no-cwd disables
        #     this behaviour.
        #
        #   --share=SPEC       for containers, share writable host file system
        #                      according to SPEC
        #
        #   --share="$PWD"=/bitcoin
        #
        #                     maps our current working directory to /bitcoin
        #                     inside the isolated container, which we later cd
        #                     into.
        #
        #     While we don't want to map our current working directory to the
        #     same exact path (as this introduces irreproducibility), we do want
        #     it to be at a _fixed_ path _somewhere_ inside the isolated
        #     container so that we have something to build. '/bitcoin' was
        #     chosen arbitrarily.
        #
        #   ${SOURCES_PATH:+--share="$SOURCES_PATH"}
        #
        #                     make the downloaded depends sources path available
        #                     inside the isolated container
        #
        #     The isolated container has no network access as it's in a
        #     different network namespace from the main machine, so we have to
        #     make the downloaded depends sources available to it. The sources
        #     should have been downloaded prior to this invocation.
        #
        # shellcheck disable=SC2086
        time-machine environment --manifest="${PWD}/contrib/guix/manifest.scm" \
                                 --container \
                                 --pure \
                                 --no-cwd \
                                 --share="$PWD"=/bitcoin \
                                 --expose="$(git rev-parse --git-common-dir)" \
                                 ${SOURCES_PATH:+--share="$SOURCES_PATH"} \
                                 ${ADDITIONAL_GUIX_ENVIRONMENT_FLAGS} \
                                 -- env HOST="$host" \
                                        MAX_JOBS="$MAX_JOBS" \
                                        SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:?unable to determine value}" \
                                        ${V:+V=1} \
                                        ${SOURCES_PATH:+SOURCES_PATH="$SOURCES_PATH"} \
                                      bash -c "cd /bitcoin && bash contrib/guix/libexec/build.sh"
    )

done
