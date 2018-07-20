#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Make sure we don't depend on GNU specific functionality in commands called
# from shell scripts.

export LC_ALL=C

EXIT_CODE=0

# BSD grep does not support empty subexpressions
#
# macOS High Sierra:
#
# $ grep --version
# grep (BSD grep) 2.5.1-FreeBSD
# $ echo foo | grep -E '^foo(_r|_s)?$'
# foo
# $ echo foo | grep -E '^foo(|_r|_s)$'
# grep: empty (sub)expression
#
# Ubuntu 18.04 LTS:
#
# $ grep --version
# grep (GNU grep) 3.1
# $ echo foo | grep -E '^foo(_r|_s)?$'
# foo
# $ echo foo | grep -E '^foo(|_r|_s)$'
# foo

if git grep -E 'grep .*(\([^)]*\|\)|\(\|[^)]*\)|\([^)]*\|\|[^)]*\))' -- "*.sh" ":!test/lint/lint-shell-bsd-compat.sh"; then
    echo "^ BSD grep does not support empty subexpressions. Please reformulate the regexp."
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
