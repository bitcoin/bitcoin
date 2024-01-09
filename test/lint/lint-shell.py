#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check for shellcheck warnings in shell scripts.
"""

import subprocess
import re
import sys

# Disabled warnings:
DISABLED = [
    'SC2162', # read without -r will mangle backslashes.
]

def check_shellcheck_install():
    try:
        subprocess.run(['shellcheck', '--version'], stdout=subprocess.DEVNULL, check=True)
    except FileNotFoundError:
        print('Skipping shell linting since shellcheck is not installed.')
        sys.exit(0)

def get_files(command):
    output = subprocess.run(command, stdout=subprocess.PIPE, text=True)
    files = output.stdout.split('\n')

    # remove whitespace element
    files = list(filter(None, files))
    return files

def main():
    check_shellcheck_install()

    # build the `exclude` flag
    exclude = '--exclude=' + ','.join(DISABLED)

    # build the `sourced files` list
    sourced_files_cmd = [
        'git',
        'grep',
        '-El',
        r'^# shellcheck shell=',
    ]
    sourced_files = get_files(sourced_files_cmd)

    # build the `guix files` list
    guix_files_cmd = [
        'git',
        'grep',
        '-El',
        r'^#!\/usr\/bin\/env bash',
        '--',
        'contrib/guix',
        'contrib/shell',
    ]
    guix_files = get_files(guix_files_cmd)

    # build the other script files list
    files_cmd = [
        'git',
        'ls-files',
        '--',
        '*.sh',
    ]
    files = get_files(files_cmd)
    reg = re.compile(r'src/[leveldb,secp256k1,minisketch]')

    def should_exclude(fname: str) -> bool:
        return bool(reg.match(fname))

    # remove everything that doesn't match this regex
    files[:] = [file for file in files if not should_exclude(file)]

    # build the `shellcheck` command
    shellcheck_cmd = [
        'shellcheck',
        '--external-sources',
        '--check-sourced',
        '--source-path=SCRIPTDIR',
    ]
    shellcheck_cmd.append(exclude)
    shellcheck_cmd.extend(sourced_files)
    shellcheck_cmd.extend(guix_files)
    shellcheck_cmd.extend(files)

    # run the `shellcheck` command
    try:
        subprocess.check_call(shellcheck_cmd)
    except subprocess.CalledProcessError:
        sys.exit(1)

if __name__ == '__main__':
    main()
