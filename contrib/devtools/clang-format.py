#!/usr/bin/env python
'''
Wrapper script for clang-format

Copyright (c) 2015 MarcoFalke
Copyright (c) 2015 The Bitcoin Core developers
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''

import os
import sys
import subprocess

tested_versions = ['3.6.0', '3.6.1', '3.6.2'] # A set of versions known to produce the same output
accepted_file_extensions = ('.h', '.cpp') # Files to format

def check_clang_format_version(clang_format_exe):
    try:
        output = subprocess.check_output([clang_format_exe, '-version'])
        for ver in tested_versions:
            if ver in output:
                print "Detected clang-format version " + ver
                return
        raise RuntimeError("Untested version: " + output)
    except Exception as e:
        print 'Could not verify version of ' + clang_format_exe + '.'
        raise e

def check_command_line_args(argv):
    required_args = ['{clang-format-exe}', '{files}']
    example_args = ['clang-format-3.x', 'src/main.cpp', 'src/wallet/*']

    if(len(argv) < len(required_args) + 1):
        for word in (['Usage:', argv[0]] + required_args):
            print word,
        print ''
        for word in (['E.g:', argv[0]] + example_args):
            print word,
        print ''
        sys.exit(1)

def run_clang_format(clang_format_exe, files):
    for target in files:
        if os.path.isdir(target):
            for path, dirs, files in os.walk(target):
                run_clang_format(clang_format_exe, (os.path.join(path, f) for f in files))
        elif target.endswith(accepted_file_extensions):
            print "Format " + target
            subprocess.check_call([clang_format_exe, '-i', '-style=file', target], stdout=open(os.devnull, 'wb'), stderr=subprocess.STDOUT)
        else:
            print "Skip " + target

def main(argv):
    check_command_line_args(argv)
    clang_format_exe = argv[1]
    files = argv[2:]
    check_clang_format_version(clang_format_exe)
    run_clang_format(clang_format_exe, files)

if __name__ == "__main__":
    main(sys.argv)
