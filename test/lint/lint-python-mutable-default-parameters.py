#!/usr/bin/env python3
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Detect when a mutable list or dict is used as a default parameter value in a Python function.
"""

import subprocess
import sys


def main():
    command = [
        "git",
        "grep",
        "-E",
        r"^\s*def [a-zA-Z0-9_]+\(.*=\s*(\[|\{)",
        "--",
        "*.py",
    ]
    output = subprocess.run(command, stdout=subprocess.PIPE, text=True)
    if len(output.stdout) > 0:
        error_msg = (
            "A mutable list or dict seems to be used as default parameter value:\n\n"
            f"{output.stdout}\n"
            f"{example()}"
        )
        print(error_msg)
        sys.exit(1)
    else:
        sys.exit(0)


def example():
    return """This is how mutable list and dict default parameter values behave:

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
([2], {2: True})"""


if __name__ == "__main__":
    main()
