#!/usr/bin/env python3
#
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for new lines in diff that introduce trailing whitespace or
# tab characters instead of spaces.

# We can't run this check unless we know the commit range for the PR.

import argparse
import os
import re
import sys

from subprocess import check_output

EXCLUDED_DIRS = ["depends/patches/",
                 "contrib/guix/patches/",
                 "src/leveldb/",
                 "src/crc32c/",
                 "src/secp256k1/",
                 "src/minisketch/",
                 "doc/release-notes/",
                 "src/qt/locale",
                 "src/dashbls/",
                 "src/immer/"]

def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="""
            Check for new lines in diff that introduce trailing whitespace
            or tab characters instead of spaces in unstaged changes, the
            previous n commits, or a commit-range.
        """,
        epilog=f"""
            You can manually set the commit-range with the COMMIT_RANGE
            environment variable (e.g. "COMMIT_RANGE='47ba2c3...ee50c9e'
            {sys.argv[0]}"). Defaults to current merge base when neither
            prev-commits nor the environment variable is set.
        """)

    parser.add_argument("--prev-commits", "-p", required=False, help="The previous n commits to check")

    return parser.parse_args()


def report_diff(selection):
    filename = ""
    seen = False
    seenln = False

    print("The following changes were suspected:")

    for line in selection:
        if re.match(r"^diff", line):
            filename = line
            seen = False
        elif re.match(r"^@@", line):
            linenumber = line
            seenln = False
        else:
            if not seen:
                # The first time a file is seen with trailing whitespace or a tab character, we print the
                # filename (preceded by a newline).
                print("")
                print(filename)
                seen = True
            if not seenln:
                print(linenumber)
                seenln = True
            print(line)


def get_diff(commit_range, check_only_code):
    exclude_args = [":(exclude)" + dir for dir in EXCLUDED_DIRS]

    if check_only_code:
        what_files = ["*.cpp", "*.h", "*.md", "*.py", "*.sh"]
    else:
        what_files = ["."]

    diff = check_output(["git", "diff", "-U0", commit_range, "--"] + what_files + exclude_args, text=True, encoding="utf8")

    return diff


def main():
    args = parse_args()

    if not os.getenv("COMMIT_RANGE"):
        if args.prev_commits:
            commit_range = "HEAD~" + args.prev_commits + "...HEAD"
        else:
            # This assumes that the target branch of the pull request will be master.
            merge_base = check_output(["git", "merge-base", "HEAD", "dev-4.x"], text=True, encoding="utf8").rstrip("\n")
            commit_range = merge_base + "..HEAD"
    else:
        commit_range = os.getenv("COMMIT_RANGE")
        if commit_range == "SKIP_EMPTY_NOT_A_PR":
            sys.exit(0)

    whitespace_selection = []
    tab_selection = []

    # Check if trailing whitespace was found in the diff.
    for line in get_diff(commit_range, check_only_code=False).splitlines():
        if re.match(r"^(diff --git|\@@|^\+.*\s+$)", line):
            whitespace_selection.append(line)

    whitespace_additions = [i for i in whitespace_selection if i.startswith("+")]

    # Check if tab characters were found in the diff.
    for line in get_diff(commit_range, check_only_code=True).splitlines():
        if re.match(r"^(diff --git|\@@|^\+.*\t)", line):
            tab_selection.append(line)

    tab_additions = [i for i in tab_selection if i.startswith("+")]

    ret = 0

    if len(whitespace_additions) > 0:
        print("This diff appears to have added new lines with trailing whitespace.")
        report_diff(whitespace_selection)
        ret = 1

    if len(tab_additions) > 0:
        print("This diff appears to have added new lines with tab characters instead of spaces.")
        report_diff(tab_selection)
        ret = 1

    sys.exit(ret)


if __name__ == "__main__":
    main()
