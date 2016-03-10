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
BINARIES = ['bitcoind', 'bitcoin-tx', 'bitcoin-cli', 'qt/bitcoin-qt'] # TODO libconsensus?

def run_copy_tagged_binary(src, target):
    print "Copying " + src + " to " + target
    subprocess.check_call(['cp', src, target])

def loop_copy_tagged_binaries(binaries, petname):
    last_commit = os.popen('git log -n 1 --format="%H"').read()[0:7]
    for binary in binaries:
        run_copy_tagged_binary(SRC + binary, TARGET + binary + petname + '-' + last_commit)

def main(argv):
    if (argv and len(argv) > 1 and argv[1]):
        petname = '-' + argv[1]
    else:
        petname = ''

    loop_copy_tagged_binaries(BINARIES, petname)

if __name__ == "__main__":
    main(sys.argv)
