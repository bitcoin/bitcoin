#!/usr/bin/env python3
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Make sure all shell scripts explicitly opt out of locale dependence using
"export LC_ALL=C" or "export LC_ALL=C.UTF-8", which also enables UTF-8 mode in
Python. See: https://docs.python.org/3/library/os.html#python-utf-8-mode
"""

import subprocess
import sys
import re

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
        return subprocess.check_output(command, stderr = subprocess.STDOUT, text=True).splitlines()
    except subprocess.CalledProcessError as e:
        if e.returncode > 1: # return code is 1 when match is empty
            print(e.output, end='')
            sys.exit(1)
        return []

def main():
    exit_code = 0
    shell_files = get_shell_files_list()
    for file_path in shell_files:
        if re.search('src/(secp256k1|minisketch)/', file_path):
            continue

        with open(file_path, 'r') as file_obj:
            contents = file_obj.read()

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

