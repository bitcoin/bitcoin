#!/usr/bin/env bash
export LC_ALL=C
set -e -o pipefail

###################
## Sanity Checks ##
###################

################
# Check 1: Make sure that we can invoke required tools
################
for cmd in git make guix cat mkdir; do
    if ! command -v "$cmd" > /dev/null 2>&1; then
        echo "ERR: This script requires that '$cmd' is installed and available in your \$PATH"
        exit 1
    fi
done

################
# Check 2: Make sure GUIX_BUILD_OPTIONS is empty
################
#
# GUIX_BUILD_OPTIONS is an environment variable recognized by guix commands that
# can perform builds. This seems like what we want instead of
# ADDITIONAL_GUIX_COMMON_FLAGS, but the value of GUIX_BUILD_OPTIONS is actually
# _appended_ to normal command-line options. Meaning that they will take
# precedence over the command-specific ADDITIONAL_GUIX_<CMD>_FLAGS.
#
# This seems like a poor user experience. Thus we check for GUIX_BUILD_OPTIONS's
# existence here and direct users of this script to use our (more flexible)
# custom environment variables.
if [ -n "$GUIX_BUILD_OPTIONS" ]; then
cat << EOF
Error: Environment variable GUIX_BUILD_OPTIONS is not empty:
  '$GUIX_BUILD_OPTIONS'

Unfortunately this script is incompatible with GUIX_BUILD_OPTIONS, please unset
GUIX_BUILD_OPTIONS and use ADDITIONAL_GUIX_COMMON_FLAGS to set build options
across guix commands or ADDITIONAL_GUIX_<CMD>_FLAGS to set build options for a
specific guix command.

See contrib/guix/README.md for more details.
EOF
exit 1
fi

################
# Check 3: Make sure that we're not in a dirty worktree
################
if ! git diff-index --quiet HEAD -- && [ -z "$FORCE_DIRTY_WORKTREE" ]; then
cat << EOF
ERR: The current git worktree is dirty, which may lead to broken builds.

     Aborting...

Hint: To make your git worktree clean, You may want to:
      1. Commit your changes,
      2. Stash your changes, or
      3. Set the 'FORCE_DIRTY_WORKTREE' environment variable if you insist on
         using a dirty worktree
EOF
exit 1
else
    GIT_COMMIT=$(git rev-parse --short=12 HEAD)
fi

################
# Check 4: Make sure that build directories do not exist
################

# Default to building for all supported HOSTs (overridable by environment)
export HOSTS="${HOSTS:-x86_64-linux-gnu arm-linux-gnueabihf aarch64-linux-gnu riscv64-linux-gnu
                       x86_64-w64-mingw32
                       x86_64-apple-darwin18}"

DISTSRC_BASE="${DISTSRC_BASE:-${PWD}}"

# Usage: distsrc_for_host HOST
#
#   HOST: The current platform triple we're building for
#
distsrc_for_host() {
    echo "${DISTSRC_BASE}/distsrc-${GIT_COMMIT}-${1}"
}

# Accumulate a list of build directories that already exist...
hosts_distsrc_exists=""
for host in $HOSTS; do
    if [ -e "$(distsrc_for_host "$host")" ]; then
        hosts_distsrc_exists+=" ${host}"
    fi
done

if [ -n "$hosts_distsrc_exists" ]; then
# ...so that we can print them out nicely in an error message
cat << EOF
ERR: Build directories for this commit already exist for the following platform
     triples you're attempting to build, probably because of previous builds.
     Please remove, or otherwise deal with them prior to starting another build.

     Aborting...

EOF
for host in $hosts_distsrc_exists; do
    echo "     ${host} '$(distsrc_for_host "$host")'"
done
exit 1
else

    mkdir -p "$DISTSRC_BASE"
fi

################
# Check 5: When building for darwin, make sure that the macOS SDK exists
################

for host in $HOSTS; do
    case "$host" in
        *darwin*)
            OSX_SDK="$(make -C "${PWD}/depends" --no-print-directory HOST="$host" print-OSX_SDK | sed 's@^[^=]\+=[[:space:]]\+@@g')"
            if [ -e "$OSX_SDK" ]; then
                echo "Found macOS SDK at '${OSX_SDK}', using..."
            else
                echo "macOS SDK does not exist at '${OSX_SDK}', please place the extracted, untarred SDK there to perform darwin builds, exiting..."
                exit 1
            fi
            ;;
    esac
done

#########
# Setup #
#########

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
    # shellcheck disable=SC2086
    guix time-machine --url=https://github.com/dongcarl/guix.git \
                      --commit=b066c25026f21fb57677aa34692a5034338e7ee3 \
                      --max-jobs="$MAX_JOBS" \
                      ${SUBSTITUTE_URLS:+--substitute-urls="$SUBSTITUTE_URLS"} \
                      ${ADDITIONAL_GUIX_COMMON_FLAGS} ${ADDITIONAL_GUIX_TIMEMACHINE_FLAGS} \
                      -- "$@"
}

# Make sure an output directory exists for our builds
OUTDIR="${OUTDIR:-${PWD}/output}"
[ -e "$OUTDIR" ] || mkdir -p "$OUTDIR"

#########
# Build #
#########

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

# Deterministically build Bitcoin Core
# shellcheck disable=SC2153
for host in $HOSTS; do

    # Display proper warning when the user interrupts the build
    trap 'int_trap ${host}' INT

    (
        # Required for 'contrib/guix/manifest.scm' to output the right manifest
        # for the particular $HOST we're building for
        export HOST="$host"

        # shellcheck disable=SC2030
cat << EOF
INFO: Building commit ${GIT_COMMIT:?not set} for platform triple ${HOST:?not set}:
      ...using reference timestamp: ${SOURCE_DATE_EPOCH:?not set}
      ...running at most ${MAX_JOBS:?not set} jobs
      ...from worktree directory: '${PWD}'
          ...bind-mounted in container to: '/bitcoin'
      ...in build directory: '$(distsrc_for_host "$HOST")'
          ...bind-mounted in container to: '$(DISTSRC_BASE=/distsrc-base && distsrc_for_host "$HOST")'
      ...outputting in: '${OUTDIR:?not set}'
          ...bind-mounted in container to: '/outdir'
EOF

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
        #  ${SUBSTITUTE_URLS:+--substitute-urls="$SUBSTITUTE_URLS"}
        #
        #                     fetch substitute from SUBSTITUTE_URLS if they are
        #                     authorized
        #
        #    Depending on the user's security model, it may be desirable to use
        #    substitutes (pre-built packages) from servers that the user trusts.
        #    Please read the README.md in the same directory as this file for
        #    more information.
        #
        # shellcheck disable=SC2086,SC2031
        time-machine environment --manifest="${PWD}/contrib/guix/manifest.scm" \
                                 --container \
                                 --pure \
                                 --no-cwd \
                                 --share="$PWD"=/bitcoin \
                                 --share="$DISTSRC_BASE"=/distsrc-base \
                                 --share="$OUTDIR"=/outdir \
                                 --expose="$(git rev-parse --git-common-dir)" \
                                 ${SOURCES_PATH:+--share="$SOURCES_PATH"} \
                                 --max-jobs="$MAX_JOBS" \
                                 ${SUBSTITUTE_URLS:+--substitute-urls="$SUBSTITUTE_URLS"} \
                                 ${ADDITIONAL_GUIX_COMMON_FLAGS} ${ADDITIONAL_GUIX_ENVIRONMENT_FLAGS} \
                                 -- env HOST="$host" \
                                        MAX_JOBS="$MAX_JOBS" \
                                        SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:?unable to determine value}" \
                                        ${V:+V=1} \
                                        ${SOURCES_PATH:+SOURCES_PATH="$SOURCES_PATH"} \
                                        DISTSRC="$(DISTSRC_BASE=/distsrc-base && distsrc_for_host "$HOST")" \
                                        OUTDIR=/outdir \
                                      bash -c "cd /bitcoin && bash contrib/guix/libexec/build.sh"
    )

done
