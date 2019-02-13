#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for duplicate includes.
# Guard against accidental introduction of new Boost dependencies.
# Check includes: Check for duplicate includes. Enforce bracket syntax includes.

export LC_ALL=C
IGNORE_REGEXP="/(leveldb|secp256k1|univalue|crc32c)/"

# cd to root folder of git repo for git ls-files to work properly
cd "$(dirname $0)/../.." || exit 1

filter_suffix() {
    git ls-files | grep -E "^src/.*\.${1}"'$' | grep -Ev "${IGNORE_REGEXP}"
}

EXIT_CODE=0

for HEADER_FILE in $(filter_suffix h); do
    DUPLICATE_INCLUDES_IN_HEADER_FILE=$(grep -E "^#include " < "${HEADER_FILE}" | sort | uniq -d)
    if [[ ${DUPLICATE_INCLUDES_IN_HEADER_FILE} != "" ]]; then
        echo "Duplicate include(s) in ${HEADER_FILE}:"
        echo "${DUPLICATE_INCLUDES_IN_HEADER_FILE}"
        echo
        EXIT_CODE=1
    fi
done

for CPP_FILE in $(filter_suffix cpp); do
    DUPLICATE_INCLUDES_IN_CPP_FILE=$(grep -E "^#include " < "${CPP_FILE}" | sort | uniq -d)
    if [[ ${DUPLICATE_INCLUDES_IN_CPP_FILE} != "" ]]; then
        echo "Duplicate include(s) in ${CPP_FILE}:"
        echo "${DUPLICATE_INCLUDES_IN_CPP_FILE}"
        echo
        EXIT_CODE=1
    fi
done

INCLUDED_CPP_FILES=$(git grep -E "^#include [<\"][^>\"]+\.cpp[>\"]" -- "*.cpp" "*.h")
if [[ ${INCLUDED_CPP_FILES} != "" ]]; then
    echo "The following files #include .cpp files:"
    echo "${INCLUDED_CPP_FILES}"
    echo
    EXIT_CODE=1
fi

EXPECTED_BOOST_INCLUDES=(
    boost/algorithm/string.hpp
    boost/algorithm/string/classification.hpp
    boost/algorithm/string/replace.hpp
    boost/algorithm/string/split.hpp
    boost/date_time/posix_time/posix_time.hpp
    boost/filesystem.hpp
    boost/filesystem/fstream.hpp
    boost/multi_index/hashed_index.hpp
    boost/multi_index/ordered_index.hpp
    boost/multi_index/sequenced_index.hpp
    boost/multi_index_container.hpp
    boost/optional.hpp
    boost/preprocessor/cat.hpp
    boost/preprocessor/stringize.hpp
    boost/process.hpp
    boost/signals2/connection.hpp
    boost/signals2/optional_last_value.hpp
    boost/signals2/signal.hpp
    boost/test/unit_test.hpp
    boost/thread/condition_variable.hpp
    boost/thread/mutex.hpp
    boost/thread/shared_mutex.hpp
    boost/thread/thread.hpp
    boost/variant.hpp
    boost/variant/apply_visitor.hpp
    boost/variant/static_visitor.hpp
)

for BOOST_INCLUDE in $(git grep '^#include <boost/' -- "*.cpp" "*.h" | cut -f2 -d: | cut -f2 -d'<' | cut -f1 -d'>' | sort -u); do
    IS_EXPECTED_INCLUDE=0
    for EXPECTED_BOOST_INCLUDE in "${EXPECTED_BOOST_INCLUDES[@]}"; do
        if [[ "${BOOST_INCLUDE}" == "${EXPECTED_BOOST_INCLUDE}" ]]; then
            IS_EXPECTED_INCLUDE=1
            break
        fi
    done
    if [[ ${IS_EXPECTED_INCLUDE} == 0 ]]; then
        EXIT_CODE=1
        echo "A new Boost dependency in the form of \"${BOOST_INCLUDE}\" appears to have been introduced:"
        git grep "${BOOST_INCLUDE}" -- "*.cpp" "*.h"
        echo
    fi
done

for EXPECTED_BOOST_INCLUDE in "${EXPECTED_BOOST_INCLUDES[@]}"; do
    if ! git grep -q "^#include <${EXPECTED_BOOST_INCLUDE}>" -- "*.cpp" "*.h"; then
        echo "Good job! The Boost dependency \"${EXPECTED_BOOST_INCLUDE}\" is no longer used."
        echo "Please remove it from EXPECTED_BOOST_INCLUDES in $0"
        echo "to make sure this dependency is not accidentally reintroduced."
        echo
        EXIT_CODE=1
    fi
done

QUOTE_SYNTAX_INCLUDES=$(git grep '^#include "' -- "*.cpp" "*.h" | grep -Ev "${IGNORE_REGEXP}")
if [[ ${QUOTE_SYNTAX_INCLUDES} != "" ]]; then
    echo "Please use bracket syntax includes (\"#include <foo.h>\") instead of quote syntax includes:"
    echo "${QUOTE_SYNTAX_INCLUDES}"
    echo
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
