#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Make sure all shell scripts are:
a.) explicitly opt out of locale dependence using
    "export LC_ALL=C" or "export LC_ALL=C.UTF-8", or
b.) explicitly opt in to locale dependence using the annotation below.
"""

import subprocess
import sys
import re

OPT_IN_LINE = '# This script is intentionally locale dependent by not setting \"export LC_ALL=C\"'

OPT_OUT_LINES = [
    'export LC_ALL=C',
    'export LC_ALL=C.UTF-8',
]

def get_shell_files_list():
    command = [
        'git',
        'ls-files',
        '--',
        '*.sh',
    ]
    try:
        return subprocess.check_output(command, stderr = subprocess.STDOUT).decode('utf-8').splitlines()
    except subprocess.CalledProcessError as e:
        if e.returncode > 1: # return code is 1 when match is empty
            print(e.output.decode('utf-8'), end='')
            sys.exit(1)
        return []

def main():
    exit_code = 0
    shell_files = get_shell_files_list()
    for file_path in shell_files:
        if re.search('src/(secp256k1|minisketch)/', file_path):
            continue

        with open(file_path, 'r', encoding='utf-8') as file_obj:
            contents = file_obj.read()

        if OPT_IN_LINE in contents:
            continue

        non_comment_pattern = re.compile(r'^\s*((?!#).+)$', re.MULTILINE)
        non_comment_lines = re.findall(non_comment_pattern, contents)
        if not non_comment_lines:
            continue

        first_non_comment_line = non_comment_lines[0]
        if first_non_comment_line not in OPT_OUT_LINES:
            print(f'Missing "export LC_ALL=C" (to avoid locale dependence) as first non-comment non-empty line in {file_path}')
            exit_code = 1

    return sys.exit(exit_code)

if __name__ == '__main__':
    main()

