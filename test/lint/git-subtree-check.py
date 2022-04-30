#!/usr/bin/env python3
#
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Checks that a certain prefix is pure subtree, and optionally whether the
# referenced commit is present in any fetched remote.

import argparse
import sys
import subprocess

def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser (
        description="""
            Checks that a certain prefix is pure subtree, and optionally whether the
            referenced commit is present in any fetched remote.
        """,
        epilog="""
        To do a full check with `-r`, make sure that you have fetched the upstream
        repository branch in which the subtree is maintained:
        (e.g. git fetch https://github.com/bitcoin-core/secp256k1.git
        test/lint/git-subtree-check.sh -r src/secp256k1)
        """
    )

    parser.add_argument('DIR', help="The prefix within the repository to check.")
    parser.add_argument('COMMIT', nargs='?', default='HEAD', help="The commit to check, if it is not provided, HEAD will be used.")
    parser.add_argument("-r", "--repository", action='store_true', required=False, help="Check that subtree commit is present in repository.")

    return parser.parse_args()

def find_latest_squash(dir, commit):
    sq = main = sub = None
    stdout = subprocess.check_output(["git", "log", "--grep=^git-subtree-dir: " + dir + "/*$", "--pretty=format: START %H%n%s%n%n%b%nEND%n", commit], universal_newlines=True, encoding="utf8")
    for line in stdout.splitlines():
        if "START" in line:
            sq = line.lstrip().split(' ')[1]
        if line.startswith("git-subtree-mainline:"):
            main = line.split(':', 2)[1].strip(' ')
        if line.startswith("git-subtree-split:"):
            sub = line.split(':', 2)[1].strip(' ')
        if "END" in line:
            if sub:
                if main:
                    sq = sub
                return(f"{sq} {sub}")
            sq = main = sub = None

def main():
    args = parse_args()

    # find the latest subtree update
    latest_squash = find_latest_squash(args.DIR.rstrip('/'), args.COMMIT)
    if not latest_squash:
        print(f"Error: {args.DIR} is not a subtree")
        sys.exit(2)

    old = latest_squash.split(' ')[0]
    rev = latest_squash.split(' ')[1]

    # get the tree in the current commit
    tree_contents = subprocess.Popen(["git", "ls-tree", "-d", args.COMMIT, args.DIR], stdout=subprocess.PIPE, universal_newlines=True, encoding="utf8")
    tree_actual = subprocess.check_output(["head",  "-n 1"], stdin=tree_contents.stdout, universal_newlines=True, encoding="utf8")
    if not tree_actual:
        print(f"FAIL: subtree directory {args.DIR} not found in {args.COMMIT}")
        sys.exit(1)

    tree_actual_type = tree_actual.split()[1]
    tree_actual_tree = tree_actual.split()[2]

    if tree_actual_type != "tree":
        print(f"FAIL: subtree directory {args.DIR} is not a tree in {args.COMMIT}")
        sys.exit(1)

    print(f"{args.DIR} in {args.COMMIT} currently refers to {tree_actual_type} {tree_actual_tree}")

    # get tree at the time of the last subtree update
    tree_commit = subprocess.check_output(["git", "show", "-s", "--format=%T", old], universal_newlines=True, encoding="utf8").strip()
    print(f"{args.DIR} in {args.COMMIT} was last updated in commit {old} (tree {tree_commit})")

    # compare the tree at the time of the last aubtree update with the actual tree
    if tree_actual_tree != tree_commit:
        print(subprocess.check_output(["git", "diff", tree_commit, tree_actual_tree], universal_newlines=True, encoding="utf8"))
        print("FAIL: subtree directory was touched without subtree merge")
        sys.exit(1)

    if args.repository:
        # get the tree in the subtree the commit referred to
        if str(subprocess.check_output(["git", "cat-file", "-t", rev], stderr=subprocess.DEVNULL, universal_newlines=True, encoding="utf8")).strip() != "commit":
            print(f"subtree commit {rev} unavailable: cannot compare. Did you add and fetch the remote?")
            sys.exit(1)

        tree_subtree = subprocess.check_output(["git", "show", "-s", "--format=%T", rev], universal_newlines=True, encoding="utf8").strip()
        print(f"{args.DIR} in {args.COMMIT} was last updated to upstream commit {rev} (tree {tree_subtree})")

        # compare tree in subtree commit to the actual tree
        if tree_actual_tree != tree_subtree:
            print("FAIL: subtree update commit differs from upstream tree!")
            sys.exit(1)

    print("GOOD")

if __name__ == "__main__":
    main()
