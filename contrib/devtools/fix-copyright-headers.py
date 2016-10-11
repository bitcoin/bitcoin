#!/usr/bin/env python3
"""
Run this script to update all the copyright headers of files
that were changed this year.

For example:

// Copyright (c) 2009-2012 The Bitcoin Core developers

it will change it to

// Copyright (c) 2009-2015 The Bitcoin Core developers
"""
import subprocess
import time
import re

CMD_GIT_LIST_FILES = ['git', 'ls-files']
CMD_GIT_DATE = ['git', 'log', '--format=%ad', '--date=short', '-1']
CMD_PERL_REGEX = ['perl', '-pi', '-e']
REGEX_TEMPLATE = 's/(20\\d\\d)(?:-20\\d\\d)? The Bitcoin/$1-%s The Bitcoin/'

FOLDERS = ["qa/", "src/"]
EXTENSIONS = [".cpp",".h", ".py"]


def get_git_date(file_path):
    d = subprocess.run(CMD_GIT_DATE + [file_path],
                       stdout=subprocess.PIPE,
                       check=True,
                       universal_newlines=True).stdout
    # yyyy-mm-dd
    return d.split('-')[0]


def skip_file(file_path):
    for ext in EXTENSIONS:
        if file_path.endswith(ext):
            return False
    else:
        return True

if __name__ == "__main__":
    year = str(time.gmtime()[0])
    regex_current = re.compile("%s The Bitcoin" % year)
    n = 1
    for folder in FOLDERS:
        for file_path in subprocess.run(
            CMD_GIT_LIST_FILES + [folder],
            stdout=subprocess.PIPE,
            check=True,
            universal_newlines=True
        ).stdout.split("\n"):
            if skip_file(file_path):
                # print(file_path, "(skip)")
                continue
            git_date = get_git_date(file_path)
            if not year == git_date:
                # print(file_path, year, "(skip)")
                continue
            if regex_current.search(open(file_path, "r").read()) is not None:
                # already up to date
                # print(file_path, year, "(skip)")
                continue
            print(n, file_path, "(update to %s)" % year)
            subprocess.run(CMD_PERL_REGEX + [REGEX_TEMPLATE % year, file_path], check=True)
            n = n + 1
