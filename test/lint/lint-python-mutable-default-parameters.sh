#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Detect when a mutable list or dict is used as a default parameter value in a Python function.

export LC_ALL=C
EXIT_CODE=0
OUTPUT=$(git grep -E '^\s*def [a-zA-Z0-9_]+\(.*=\s*(\[|\{)' -- "*.py")
if [[ ${OUTPUT} != "" ]]; then
    echo "A mutable list or dict seems to be used as default parameter value:"
    echo
    echo "${OUTPUT}"
    echo
    cat << EXAMPLE
This is how mutable list and dict default parameter values behave:

>>> def f(i, j=[], k={}):
...     j.append(i)
...     k[i] = True
...     return j, k
...
>>> f(1)
([1], {1: True})
>>> f(1)
([1, 1], {1: True})
>>> f(2)
([1, 1, 2], {1: True, 2: True})

The intended behaviour was likely:

>>> def f(i, j=None, k=None):
...     if j is None:
...         j = []
...     if k is None:
...         k = {}
...     j.append(i)
...     k[i] = True
...     return j, k
...
>>> f(1)
([1], {1: True})
>>> f(1)
([1], {1: True})
>>> f(2)
([2], {2: True})
EXAMPLE
    EXIT_CODE=1
fi
exit ${EXIT_CODE}
