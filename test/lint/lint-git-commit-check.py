#!/usr/bin/env python3
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Linter to check that commit messages have a new line before the body
# or no body at all

import argparse
import os
import sys

from subprocess import check_output


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="""
            Linter to check that commit messages have a new line before
            the body or no body at all.
        """,
        epilog=f"""
            You can manually set the commit-range with the COMMIT_RANGE
            environment variable (e.g. "COMMIT_RANGE='47ba2c3...ee50c9e'
            {sys.argv[0]}"). Defaults to current merge base when neither
            prev-commits nor the environment variable is set.
        """)

    parser.add_argument("--prev-commits", "-p", required=False, help="The previous n commits to check")

    return parser.parse_args()


def main():
    args = parse_args()
    exit_code = 0

    if not os.getenv("COMMIT_RANGE"):
        if args.prev_commits:
            commit_range = "HEAD~" + args.prev_commits + "...HEAD"
        else:
            # This assumes that the target branch of the pull request will be dev-4.x.
            merge_base = check_output(["git", "merge-base", "HEAD", "dev-4.x"], text=True, encoding="utf8").rstrip("\n")
            commit_range = merge_base + "..HEAD"
    else:
        commit_range = os.getenv("COMMIT_RANGE")
        if commit_range == "SKIP_EMPTY_NOT_A_PR":
            sys.exit(0)

    commit_hashes = check_output(["git", "log", commit_range, "--format=%H"], text=True, encoding="utf8").splitlines()

    for hash in commit_hashes:
        commit_info = check_output(["git", "log", "--format=%B", "-n", "1", hash], text=True, encoding="utf8").splitlines()
        if len(commit_info) >= 2:
            if commit_info[1]:
                print(f"The subject line of commit hash {hash} is followed by a non-empty line. Subject lines should always be followed by a blank line.")
                exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
