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
# Enabale to catch all warnings
#    ".*"
# We may choose to enable these in the future
#    "Variable '.*' is assigned in constructor body. Consider performing initialization in initialization list."
#    "Member variable '.*' is not initialized in the constructor."
#    "Consider using std::copy algorithm instead of a raw loop."
#    "Consider using std::any_of algorithm instead of a raw loop."
#    "Consider using std::accumulate algorithm instead of a raw loop."
#    "Consider using std::copy algorithm instead of a raw loop."
#    "Consider using std::transform algorithm instead of a raw loop."
#    "Consider using std::find_if algorithm instead of a raw loop."
)

IGNORED_WARNINGS=(
    "src/bls/bls.h:.* Struct 'CBLSIdImplicit' has a constructor with 1 argument that is not explicit."
    "src/llmq/quorums_init.cpp:.* warning: Variable 'llmqDbTmp' is assigned a value that is never used."
    "src/llmq/quorums.cpp:.* warning: Parameter '_blsWorker' can be declared with const"

# General catchall, for some reason any value named 'hash' is viewed as never used.
    "Variable 'hash' is assigned a value that is never used."

# The following can be useful to ignore when the catch all is used
#    "Consider performing initialization in initialization list."
#    "Consider using std::transform algorithm instead of a raw loop."
#    "Consider using std::accumulate algorithm instead of a raw loop."
#    "Consider using std::any_of algorithm instead of a raw loop."
#    "Consider using std::count_if algorithm instead of a raw loop."
#    "Member variable '.*' is not initialized in the constructor."
#    "Consider using std::find_if algorithm instead of a raw loop."
)

# We should attempt to update this with all dash specific code
FILES=$(git ls-files -- "src/bls/*.cpp" "src/bls/*.h" "src/coinjoin/*.cpp" "src/coinjoin/*.h" "src/evo/*.cpp" \
                        "src/evo/*.h" "src/governance/*.cpp" "src/governance/*.h" "src/llmq/*.cpp" "src/llmq/*.h" \
                        "src/masternode/*.cpp" "src/masternode/*.h" "src/dsnotificationinterface.*" "src/spork.*" \
                        "src/rpc/coinjoin.cpp" "src/rpc/governance.cpp" "src/rpc/masternode.cpp" "src/rpc/rpcevo.cpp" \
                        "src/rpc/rpcquorums.cpp")


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
WARNINGS=$(echo "${FILES}" | \
    xargs cppcheck --enable=all -j "$(getconf _NPROCESSORS_ONLN)" --language=c++ --std=c++17 --template=gcc -D__cplusplus -DENABLE_WALLET -DCLIENT_VERSION_BUILD -DCLIENT_VERSION_IS_RELEASE -DCLIENT_VERSION_MAJOR -DCLIENT_VERSION_MINOR -DCLIENT_VERSION_REVISION -DCOPYRIGHT_YEAR -DDEBUG -DHAVE_WORKING_BOOST_SLEEP_FOR -DCHAR_BIT=8 -I src/ -q 2>&1 | sort -u | \
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
