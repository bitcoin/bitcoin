#!/usr/bin/env python
'''
Copyright (c) 2016 Jorge Timon
Copyright (c) 2016 The Bitcoin Core developers
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.

Script to copy all the binaries to a bin dir appending the last commit id and optionally a petname to the file name.
Optional parameter: an arbitrary string before the commit id.
Preconditions: Assumes bin and bin/qt dirs in calling dir.
'''

import os
import sys
import subprocess

SRC = 'src/'
TARGET = 'bin/'
BINARY_PATTERNS = ['%sd', '%s-tx', '%s-cli', 'qt/%s-qt']

def GetArg(argv, pos, default):
    if (argv and len(argv) > pos and argv[pos]):
        return argv[pos]
    else:
        return default

def run_copy_tagged_binary(src, target):
    print "Copying " + src + " to " + target
    subprocess.check_call(['cp', src, target])

def loop_copy_tagged_binaries(binaries, petname, exec_name):
    last_commit = os.popen('git log -n 1 --format="%H"').read()[0:7]
    for binary_pattern in binaries:
        binary = binary_pattern % exec_name
        run_copy_tagged_binary(SRC + binary, TARGET + binary + petname + '-' + last_commit)

def main(argv):
    petname = GetArg(argv, 1, '')
    if len(petname) > 0:
        petname = '-' +petname
    exec_name = GetArg(argv, 2, 'bitcoin')

    loop_copy_tagged_binaries(BINARY_PATTERNS, petname, exec_name)

if __name__ == "__main__":
    main(sys.argv)
