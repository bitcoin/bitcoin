#!/bin/bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for duplicate includes.
# Guard against accidental introduction of new Boost dependencies.

filter_suffix() {
    git ls-files | grep -E "^src/.*\.${1}"'$' | grep -Ev "/(leveldb|secp256k1|univalue)/"
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
    CPP_FILE=${HEADER_FILE/%\.h/.cpp}
    if [[ ! -e $CPP_FILE ]]; then
        continue
    fi
    DUPLICATE_INCLUDES_IN_HEADER_AND_CPP_FILES=$(grep -hE "^#include " <(sort -u < "${HEADER_FILE}") <(sort -u < "${CPP_FILE}") | grep -E "^#include " | sort | uniq -d)
    if [[ ${DUPLICATE_INCLUDES_IN_HEADER_AND_CPP_FILES} != "" ]]; then
        echo "Include(s) from ${HEADER_FILE} duplicated in ${CPP_FILE}:"
        echo "${DUPLICATE_INCLUDES_IN_HEADER_AND_CPP_FILES}"
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

EXPECTED_BOOST_INCLUDES=(
    boost/algorithm/string.hpp
    boost/algorithm/string/case_conv.hpp
    boost/algorithm/string/classification.hpp
    boost/algorithm/string/join.hpp
    boost/algorithm/string/predicate.hpp
    boost/algorithm/string/replace.hpp
    boost/algorithm/string/split.hpp
    boost/bind.hpp
    boost/chrono/chrono.hpp
    boost/date_time/posix_time/posix_time.hpp
    boost/filesystem.hpp
    boost/filesystem/detail/utf8_codecvt_facet.hpp
    boost/filesystem/fstream.hpp
    boost/function.hpp
    boost/interprocess/sync/file_lock.hpp
    boost/lexical_cast.hpp
    boost/lockfree/queue.hpp
    boost/multi_index/hashed_index.hpp
    boost/multi_index/ordered_index.hpp
    boost/multi_index/sequenced_index.hpp
    boost/multi_index_container.hpp
    boost/optional.hpp
    boost/pool/pool_alloc.hpp
    boost/preprocessor/cat.hpp
    boost/preprocessor/stringize.hpp
    boost/program_options/detail/config_file.hpp
    boost/program_options/parsers.hpp
    boost/scoped_array.hpp
    boost/signals2/last_value.hpp
    boost/signals2/signal.hpp
    boost/test/unit_test.hpp
    boost/test/unit_test_monitor.hpp
    boost/thread.hpp
    boost/thread/condition_variable.hpp
    boost/thread/mutex.hpp
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

exit ${EXIT_CODE}
