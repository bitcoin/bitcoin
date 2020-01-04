#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This test checks if all test files have the 755 unix permission (-rwxr-xr-x)

import os
from os import listdir

def list_files(dir, ext):
    return (f for f in listdir(dir) if f.endswith('.' + ext))

def check_files(FILES):
    for identifier in FILES:
        directory = identifier.split("/")
        extension = directory[-1]
        directory.pop(-1)
        directory = '/'.join(directory) + '/'
        listed_files = list_files(directory, extension)
        for f in listed_files:
            absolute_file = directory + f
            permission = oct(os.stat(absolute_file).st_mode)[-3:]
            assert permission == "755", "File {} has the wrong filemode ({}), please change it to 755".format(absolute_file, permission)

def main():
    # The last part in the path is the file extension
    FILES = [
        "test/functional/py",
        "test/fuzz/py",
        "test/lint/sh",
        "test/lint/py",
    ]
    check_files(FILES)

if __name__ == "__main__":
    main()

