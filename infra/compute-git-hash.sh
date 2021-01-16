#!/usr/bin/env bash
#
# Prints the short git hash (12 digits) of the currently checked out revision.
#
# If the repository is a git clone, uses git. If it is a mercurial clone uses
# mercurial with hg-git. Can be used from any CWD, but it cannot be moved out
# from the current directory position without modifying it.
#
# - prints the result on stdout
# - if no .hg or .git directory is found, exits with 1
# - if no hash can be computed, exits with 2
#
# EXAMPLE:
#     $ infra/compute-git-hash.sh
#     c272b7bf1d0b

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

errecho() {
    # prints to stderr
    >&2 echo "${@}"
}

CANDIDATE_REPO_ROOT=$(realpath "${MYDIR}/..")

if [ -d "${CANDIDATE_REPO_ROOT}/.git" ]; then
    # we are in a git repository, let's use git for computing the hash of the
    # current working copy
    REPO_TYPE="git"
    GIT_HASH=$(git -C "${CANDIDATE_REPO_ROOT}" rev-parse --short=12 HEAD)
elif [ -d "${CANDIDATE_REPO_ROOT}/.hg" ]; then
    # we are in a mercurial repository, let's use mercurial for computing the
    # hash of the current working copy
    REPO_TYPE="hg"
    GIT_HASH=$(HGPLAIN= hg log --repository "${CANDIDATE_REPO_ROOT}" -r . --template '{gitnode|short}\n')
else
    errecho "ERROR: the directory ${CANDIDATE_REPO_ROOT} does not appear to be a git or mercurial repository"
    exit 1
fi

if [ -z "${GIT_HASH}" ]; then
    errecho "ERROR: the commit hash of the working copy at ${CANDIDATE_REPO_ROOT} could not be identified"
    if [ "${REPO_TYPE}" == "hg" ]; then
        errecho ""
        errecho "Since you are running on a mercurial repository: did you recently committed without pushing?"
        errecho "If so, try to execute \"hg gexport\""
    fi
    exit 2
fi

# if everything went ok, print the hash to stdout
echo "${GIT_HASH}"
