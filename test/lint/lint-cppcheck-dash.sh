#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

export LC_ALL=C

ENABLED_CHECKS=(
    "Class '.*' has a constructor with 1 argument that is not explicit."
    "Struct '.*' has a constructor with 1 argument that is not explicit."
    "Function parameter '.*' should be passed by const reference."
    "Comparison of modulo result is predetermined"
    "Local variable '.*' shadows outer argument"
    "Redundant initialization for '.*'. The initialized value is overwritten before it is read."
    "Dereferencing '.*' after it is deallocated / released"
    "The scope of the variable '.*' can be reduced."
    "Parameter '.*' can be declared with const"
    "Variable '.*' can be declared with const"
    "Variable '.*' is assigned a value that is never used."
    "Unused variable"
    "The function '.*' overrides a function in a base class but is not marked with a 'override' specifier."
# Enable to catch all warnings
    ".*"
)

IGNORED_WARNINGS=(
    "src/bls/bls.h:.* Struct 'CBLSIdImplicit' has a constructor with 1 argument that is not explicit."
    "src/rpc/masternode.cpp:.*:21: warning: Consider using std::copy algorithm instead of a raw loop." # UniValue doesn't support std::copy
    "src/cachemultimap.h:.*: warning: Variable 'mapIt' can be declared as reference to const"
    "src/evo/simplifiedmns.cpp:.*:20: warning: Consider using std::copy algorithm instead of a raw loop."
    "src/llmq/commitment.cpp.* warning: Consider using std::all_of or std::none_of algorithm instead of a raw loop. \[useStlAlgorithm\]"
    "src/rpc/.*cpp:.*: note: Function pointer used here."
    "src/masternode/sync.cpp:.*: warning: Variable 'pnode' can be declared as pointer to const \[constVariableReference\]"
    "src/wallet/bip39.cpp.*: warning: The scope of the variable 'ssCurrentWord' can be reduced. \[variableScope\]"
    "src/.*:.*: warning: Local variable '_' shadows outer function \[shadowFunction\]"

    "src/stacktraces.cpp:.*: .*: Parameter 'info' can be declared as pointer to const"
    "src/stacktraces.cpp:.*: note: You might need to cast the function pointer here"

    "[note|warning]: Return value 'state.Invalid(.*)' is always false"
    "note: Calling function 'Invalid' returns 0"
    "note: Shadow variable"

# General catchall, for some reason any value named 'hash' is viewed as never used.
    "Variable 'hash' is assigned a value that is never used."

# The following can be useful to ignore when the catch all is used
#    "Consider performing initialization in initialization list."
    "Consider using std::transform algorithm instead of a raw loop."
    "Consider using std::accumulate algorithm instead of a raw loop."
    "Consider using std::any_of algorithm instead of a raw loop."
    "Consider using std::copy_if algorithm instead of a raw loop."
#    "Consider using std::count_if algorithm instead of a raw loop."
#    "Consider using std::find_if algorithm instead of a raw loop."
#    "Member variable '.*' is not initialized in the constructor."

    "unusedFunction"
    "unknownMacro"
    "unusedStructMember"
)

# We should attempt to update this with all dash specific code
FILES=$(git ls-files -- $(cat test/util/data/non-backported.txt))


if ! command -v cppcheck > /dev/null; then
    echo "Skipping cppcheck linting since cppcheck is not installed."
    exit 0
fi

function join_array {
    local IFS="$1"
    shift
    echo "$*"
}

ENABLED_CHECKS_REGEXP=$(join_array "|" "${ENABLED_CHECKS[@]}")
IGNORED_WARNINGS_REGEXP=$(join_array "|" "${IGNORED_WARNINGS[@]}")
FILES_REGEXP=$(join_array "|" "${FILES[@]}")
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# Check if CACHE_DIR is set and non-empty, otherwise use default .cppcheck/ directory
if [[ -n "$CACHE_DIR" ]]; then
    CPPCHECK_DIR=$CACHE_DIR/cppcheck/
else
    CPPCHECK_DIR=$SCRIPT_DIR/.cppcheck/
fi
if [ ! -d $CPPCHECK_DIR ]
then
    mkdir -p $CPPCHECK_DIR
fi
WARNINGS=$(echo "${FILES}" | \
    xargs cppcheck --enable=all --inline-suppr --suppress=missingIncludeSystem --cppcheck-build-dir=$CPPCHECK_DIR -j "$(getconf _NPROCESSORS_ONLN)" --language=c++ --std=c++17 --template=gcc -D__cplusplus -DENABLE_WALLET -DCLIENT_VERSION_BUILD -DCLIENT_VERSION_IS_RELEASE -DCLIENT_VERSION_MAJOR -DCLIENT_VERSION_MINOR -DCOPYRIGHT_YEAR -DDEBUG -DUSE_EPOLL -DCHAR_BIT=8 -I src/ -q 2>&1 | sort -u | \
    grep -E "${ENABLED_CHECKS_REGEXP}" | \
    grep -vE "${IGNORED_WARNINGS_REGEXP}" | \
    grep -E "${FILES_REGEXP}")

if [[ ${WARNINGS} != "" ]]; then
    echo "${WARNINGS}"
    echo
    echo "Advice not applicable in this specific case? Add an exception by updating"
    echo "IGNORED_WARNINGS in $0"
    # Uncomment to enforce the linter / comment to run locally
    exit 1
fi
exit 0
