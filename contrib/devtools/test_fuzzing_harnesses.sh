#!/usr/bin/env bash

export LC_ALL=C

set -e
set -o pipefail

TEST_REGEXP="${1:-}"
RUNNING_TIME="${2:-1}"

FUZZER_DIR="src/test/fuzz/"
N_FUZZERS=$(find "${FUZZER_DIR}" -type f -executable |  wc -l)
if [[ ${N_FUZZERS} == 0 ]]; then
    echo "Error: Could not find any compiled fuzz harnesses in ${FUZZER_DIR}"
    echo
    echo "Enable and build the fuzzers using:"
    echo "    \$ CC=clang CXX=clang++ ./configure --enable-fuzz --with-sanitizers=address,fuzzer,undefined"
    echo "    \$ make"
    exit 1
fi

if [[ ${TEST_REGEXP} == "" ]]; then
    echo "Found ${N_FUZZERS} fuzz harnesses to test."
    echo
fi

N_RUNS=0
for FUZZER_PATH in $(find "${FUZZER_DIR}" -type f -executable | sort); do
    FUZZER=$(basename "${FUZZER_PATH}")
    if [[ ! ${FUZZER} =~ ${TEST_REGEXP} ]]; then
        continue
    fi
    CORPUS_DIR=$(mktemp -d)
    echo "Testing fuzzer ${FUZZER} during ${RUNNING_TIME} second(s)"
    echo "A subset of reached functions:"
    ${FUZZER_PATH} -verbosity=0 -detect_leaks=0 -print_funcs=20 -timeout="${RUNNING_TIME}" -max_total_time="${RUNNING_TIME}" -print_final_stats=1 "${CORPUS_DIR}" 2>&1 | grep -vE '^(INFO: .*(files found in|max_len is not provided|seed corpus: files|A corpus is not provided)|#|"|.*/include/)' | sed "s%$(pwd)/%%g"
    N_RUNS=$((N_RUNS + 1))
    COVERED_CODE_PATHS=$(find "${CORPUS_DIR}" -type f | wc -l)
    echo "Number of unique code paths taken during fuzzing round: ${COVERED_CODE_PATHS}"
    if [[ ${COVERED_CODE_PATHS} == 0 ]]; then
        echo "No taken code paths recorded during the fuzzing round. The fuzzing setup for ${FUZZER} seems broken. Aborting."
        exit 1
    fi
    if [[ ${COVERED_CODE_PATHS} == 1 ]]; then
        echo "Only one unique code path was taken when executing ${FUZZING} during ${RUNNING_TIME} second(s) with varying inputs. Investigate how you can make more code paths reachable for the fuzzer. Aborting."
        exit 1
    fi
    rm -r "${CORPUS_DIR}"
    echo
done

if [[ ${N_RUNS} == 0 ]]; then
    echo "Could not find any fuzzers matching regexp \"${TEST_REGEXP}\"."
else
    echo "Tested fuzz harnesses seem to work as expected."
fi
