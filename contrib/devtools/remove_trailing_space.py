#!/usr/bin/env python
'''
This tool removes trailing whitespace from files passed on command line.

Usage:  remove_trailing_space.py [filepath ...]

Limitations:
- makes no backups of modified files
- modifies in place
- does not care what files you pass to it
- assumes it can keep the entire stripped file in memory

Always use only on files that are under version control!

Copyright (c) 2017 The Bitcoin Unlimited developers
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''

import sys


if __name__ == "__main__":
    for filename in sys.argv[1:]:
        lines = []
        # open file in universal newline mode, then
        # read it in, strip off trailing whitespace
        with open(filename, mode='U') as f:
            for line in f.readlines():
                lines.append(line.rstrip())

        # overwrite with stripped content
        with open(filename, mode='w') as f:
            f.seek(0)
            for line in lines:
                f.write(line + '\n')
