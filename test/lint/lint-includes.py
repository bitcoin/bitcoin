#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for duplicate includes.
# Guard against accidental introduction of new Boost dependencies.
# Check includes: Check for duplicate includes. Enforce bracket syntax includes.

import os
import re
import sys

from subprocess import check_output, CalledProcessError

from lint_ignore_dirs import SHARED_EXCLUDED_SUBTREES


EXCLUDED_DIRS = ["contrib/devtools/bitcoin-tidy/",
                ] + SHARED_EXCLUDED_SUBTREES

EXPECTED_BOOST_INCLUDES = [
                           "boost/multi_index/detail/hash_index_iterator.hpp",
                           "boost/multi_index/hashed_index.hpp",
                           "boost/multi_index/identity.hpp",
                           "boost/multi_index/indexed_by.hpp",
                           "boost/multi_index/ordered_index.hpp",
                           "boost/multi_index/sequenced_index.hpp",
                           "boost/multi_index/tag.hpp",
                           "boost/multi_index_container.hpp",
                           "boost/operators.hpp",
                           "boost/signals2/connection.hpp",
                           "boost/signals2/optional_last_value.hpp",
                           "boost/signals2/signal.hpp",
                           "boost/test/included/unit_test.hpp",
                           "boost/test/unit_test.hpp",
                           "boost/tuple/tuple.hpp",
                          ]


def get_toplevel():
    return check_output(["git", "rev-parse", "--show-toplevel"], text=True, encoding="utf8").rstrip("\n")


def list_files_by_suffix(suffixes):
    exclude_args = [":(exclude)" + dir for dir in EXCLUDED_DIRS]

    files_list = check_output(["git", "ls-files", "src"] + exclude_args, text=True, encoding="utf8").splitlines()

    return [file for file in files_list if file.endswith(suffixes)]


def find_duplicate_includes(include_list):
    tempset = set()
    duplicates = set()

    for inclusion in include_list:
        if inclusion in tempset:
            duplicates.add(inclusion)
        else:
            tempset.add(inclusion)

    return duplicates


def find_included_cpps():
    included_cpps = list()

    try:
        included_cpps = check_output(["git", "grep", "-E", r"^#include [<\"][^>\"]+\.cpp[>\"]", "--", "*.cpp", "*.h"], text=True, encoding="utf8").splitlines()
    except CalledProcessError as e:
        if e.returncode > 1:
            raise e

    return included_cpps


def find_extra_boosts():
    included_boosts = list()
    filtered_included_boost_set = set()
    exclusion_set = set()

    try:
        included_boosts = check_output(["git", "grep", "-E", r"^#include <boost/", "--", "*.cpp", "*.h"], text=True, encoding="utf8").splitlines()
    except CalledProcessError as e:
        if e.returncode > 1:
            raise e

    for boost in included_boosts:
        filtered_included_boost_set.add(re.findall(r'(?<=\<).+?(?=\>)', boost)[0])

    for expected_boost in EXPECTED_BOOST_INCLUDES:
        for boost in filtered_included_boost_set:
            if expected_boost in boost:
                exclusion_set.add(boost)

    extra_boosts = set(filtered_included_boost_set.difference(exclusion_set))

    return extra_boosts


def find_quote_syntax_inclusions():
    exclude_args = [":(exclude)" + dir for dir in EXCLUDED_DIRS]
    quote_syntax_inclusions = list()

    try:
        quote_syntax_inclusions = check_output(["git", "grep", r"^#include \"", "--", "*.cpp", "*.h"] + exclude_args, text=True, encoding="utf8").splitlines()
    except CalledProcessError as e:
        if e.returncode > 1:
            raise e

    return quote_syntax_inclusions


def main():
    exit_code = 0

    os.chdir(get_toplevel())

    # Check for duplicate includes
    for filename in list_files_by_suffix((".cpp", ".h")):
        with open(filename, "r", encoding="utf8") as file:
            include_list = [line.rstrip("\n") for line in file if re.match(r"^#include", line)]

        duplicates = find_duplicate_includes(include_list)

        if duplicates:
            print(f"Duplicate include(s) in {filename}:")
            for duplicate in duplicates:
                print(duplicate)
            print("")
            exit_code = 1

    # Check if code includes .cpp-files
    included_cpps = find_included_cpps()

    if included_cpps:
        print("The following files #include .cpp files:")
        for included_cpp in included_cpps:
            print(included_cpp)
        print("")
        exit_code = 1

    # Guard against accidental introduction of new Boost dependencies
    extra_boosts = find_extra_boosts()

    if extra_boosts:
        for boost in extra_boosts:
            print(f"A new Boost dependency in the form of \"{boost}\" appears to have been introduced:")
            print(check_output(["git", "grep", boost, "--", "*.cpp", "*.h"], text=True, encoding="utf8"))
        exit_code = 1

    # Check if Boost dependencies are no longer used
    for expected_boost in EXPECTED_BOOST_INCLUDES:
        try:
            check_output(["git", "grep", "-q", r"^#include <%s>" % expected_boost, "--", "*.cpp", "*.h"], text=True, encoding="utf8")
        except CalledProcessError as e:
            if e.returncode > 1:
                raise e
            else:
                print(f"Good job! The Boost dependency \"{expected_boost}\" is no longer used. "
                       "Please remove it from EXPECTED_BOOST_INCLUDES in test/lint/lint-includes.py "
                       "to make sure this dependency is not accidentally reintroduced.\n")
                exit_code = 1

    # Enforce bracket syntax includes
    quote_syntax_inclusions = find_quote_syntax_inclusions()
    # *Rationale*: Bracket syntax is less ambiguous because the preprocessor
    # searches a fixed list of include directories without taking location of the
    # source file into account. This allows quoted includes to stand out more when
    # the location of the source file actually is relevant.

    if quote_syntax_inclusions:
        print("Please use bracket syntax includes (\"#include <foo.h>\") instead of quote syntax includes:")
        for quote_syntax_inclusion in quote_syntax_inclusions:
            print(quote_syntax_inclusion)
        exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
