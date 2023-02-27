#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify commits against a trusted keys list."""
import argparse
import hashlib
import logging
import os
import subprocess
import sys
import time

GIT = os.getenv('GIT', 'git')

def tree_sha512sum(commit='HEAD'):
    """Calculate the Tree-sha512 for the commit.

    This is copied from github-merge.py. See https://github.com/syscoin-core/syscoin-maintainer-tools."""

    # request metadata for entire tree, recursively
    files = []
    blob_by_name = {}
    for line in subprocess.check_output([GIT, 'ls-tree', '--full-tree', '-r', commit]).splitlines():
        name_sep = line.index(b'\t')
        metadata = line[:name_sep].split()  # perms, 'blob', blobid
        assert metadata[1] == b'blob'
        name = line[name_sep + 1:]
        files.append(name)
        blob_by_name[name] = metadata[2]

    files.sort()
    # open connection to git-cat-file in batch mode to request data for all blobs
    # this is much faster than launching it per file
    p = subprocess.Popen([GIT, 'cat-file', '--batch'], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    overall = hashlib.sha512()
    for f in files:
        blob = blob_by_name[f]
        # request blob
        p.stdin.write(blob + b'\n')
        p.stdin.flush()
        # read header: blob, "blob", size
        reply = p.stdout.readline().split()
        assert reply[0] == blob and reply[1] == b'blob'
        size = int(reply[2])
        # hash the blob data
        intern = hashlib.sha512()
        ptr = 0
        while ptr < size:
            bs = min(65536, size - ptr)
            piece = p.stdout.read(bs)
            if len(piece) == bs:
                intern.update(piece)
            else:
                raise IOError('Premature EOF reading git cat-file output')
            ptr += bs
        dig = intern.hexdigest()
        assert p.stdout.read(1) == b'\n'  # ignore LF that follows blob data
        # update overall hash with file hash
        overall.update(dig.encode("utf-8"))
        overall.update("  ".encode("utf-8"))
        overall.update(f)
        overall.update("\n".encode("utf-8"))
    p.stdin.close()
    if p.wait():
        raise IOError('Non-zero return value executing git cat-file')
    return overall.hexdigest()

def main():

    # Enable debug logging if running in CI
    if 'CI' in os.environ and os.environ['CI'].lower() == "true":
        logging.getLogger().setLevel(logging.DEBUG)

    # Parse arguments
    parser = argparse.ArgumentParser(usage='%(prog)s [options] [commit id]')
    parser.add_argument('--disable-tree-check', action='store_false', dest='verify_tree', help='disable SHA-512 tree check')
    parser.add_argument('--clean-merge', type=float, dest='clean_merge', default=float('inf'), help='Only check clean merge after <NUMBER> days ago (default: %(default)s)', metavar='NUMBER')
    parser.add_argument('commit', nargs='?', default='HEAD', help='Check clean merge up to commit <commit>')
    args = parser.parse_args()

    # get directory of this program and read data files
    dirname = os.path.dirname(os.path.abspath(__file__))
    print("Using verify-commits data from " + dirname)
    with open(dirname + "/trusted-git-root", "r", encoding="utf8") as f:
        verified_root = f.read().splitlines()[0]
    with open(dirname + "/trusted-sha512-root-commit", "r", encoding="utf8") as f:
        verified_sha512_root = f.read().splitlines()[0]
    with open(dirname + "/allow-revsig-commits", "r", encoding="utf8") as f:
        revsig_allowed = f.read().splitlines()
    with open(dirname + "/allow-unclean-merge-commits", "r", encoding="utf8") as f:
        unclean_merge_allowed = f.read().splitlines()
    with open(dirname + "/allow-incorrect-sha512-commits", "r", encoding="utf8") as f:
        incorrect_sha512_allowed = f.read().splitlines()
    with open(dirname + "/trusted-keys", "r", encoding="utf8") as f:
        trusted_keys = f.read().splitlines()

    # Set commit and variables
    current_commit = args.commit
    if ' ' in current_commit:
        print("Commit must not contain spaces", file=sys.stderr)
        sys.exit(1)
    verify_tree = args.verify_tree
    no_sha1 = True
    prev_commit = ""
    initial_commit = current_commit

    # Iterate through commits
    while True:

        # Log a message to prevent Travis from timing out
        logging.debug("verify-commits: [in-progress] processing commit {}".format(current_commit[:8]))

        if current_commit == verified_root:
            print('There is a valid path from "{}" to {} where all commits are signed!'.format(initial_commit, verified_root))
            sys.exit(0)
        else:
            # Make sure this commit isn't older than trusted roots
            check_root_older_res = subprocess.run([GIT, "merge-base", "--is-ancestor", verified_root, current_commit])
            if check_root_older_res.returncode != 0:
                print(f"\"{current_commit}\" predates the trusted root, stopping!")
                sys.exit(0)

        if verify_tree:
            if current_commit == verified_sha512_root:
                print("All Tree-SHA512s matched up to {}".format(verified_sha512_root), file=sys.stderr)
                verify_tree = False
                no_sha1 = False
            else:
                # Skip the tree check if we are older than the trusted root
                check_root_older_res = subprocess.run([GIT, "merge-base", "--is-ancestor", verified_sha512_root, current_commit])
                if check_root_older_res.returncode != 0:
                    print(f"\"{current_commit}\" predates the trusted SHA512 root, disabling tree verification.")
                    verify_tree = False
                    no_sha1 = False


        os.environ['SYSCOIN_VERIFY_COMMITS_ALLOW_SHA1'] = "0" if no_sha1 else "1"
        allow_revsig = current_commit in revsig_allowed

        # Check that the commit (and parents) was signed with a trusted key
        valid_sig = False
        verify_res = subprocess.run([GIT, '-c', 'gpg.program={}/gpg.sh'.format(dirname), 'verify-commit', "--raw", current_commit], capture_output=True)
        for line in verify_res.stderr.decode().splitlines():
            if line.startswith("[GNUPG:] VALIDSIG "):
                key = line.split(" ")[-1]
                valid_sig = key in trusted_keys
            elif (line.startswith("[GNUPG:] REVKEYSIG ") or line.startswith("[GNUPG:] EXPKEYSIG ")) and not allow_revsig:
                valid_sig = False
                break
        if not valid_sig:
            if prev_commit != "":
                print("No parent of {} was signed with a trusted key!".format(prev_commit), file=sys.stderr)
                print("Parents are:", file=sys.stderr)
                parents = subprocess.check_output([GIT, 'show', '-s', '--format=format:%P', prev_commit]).decode('utf8').splitlines()[0].split(' ')
                for parent in parents:
                    subprocess.call([GIT, 'show', '-s', parent], stdout=sys.stderr)
            else:
                print("{} was not signed with a trusted key!".format(current_commit), file=sys.stderr)
            sys.exit(1)

        # Check the Tree-SHA512
        if (verify_tree or prev_commit == "") and current_commit not in incorrect_sha512_allowed:
            tree_hash = tree_sha512sum(current_commit)
            if ("Tree-SHA512: {}".format(tree_hash)) not in subprocess.check_output([GIT, 'show', '-s', '--format=format:%B', current_commit]).decode('utf8').splitlines():
                print("Tree-SHA512 did not match for commit " + current_commit, file=sys.stderr)
                sys.exit(1)

        # Merge commits should only have two parents
        parents = subprocess.check_output([GIT, 'show', '-s', '--format=format:%P', current_commit]).decode('utf8').splitlines()[0].split(' ')
        if len(parents) > 2:
            print("Commit {} is an octopus merge".format(current_commit), file=sys.stderr)
            sys.exit(1)

        # Check that the merge commit is clean
        commit_time = int(subprocess.check_output([GIT, 'show', '-s', '--format=format:%ct', current_commit]).decode('utf8').splitlines()[0])
        check_merge = commit_time > time.time() - args.clean_merge * 24 * 60 * 60  # Only check commits in clean_merge days
        allow_unclean = current_commit in unclean_merge_allowed
        if len(parents) == 2 and check_merge and not allow_unclean:
            current_tree = subprocess.check_output([GIT, 'show', '--format=%T', current_commit]).decode('utf8').splitlines()[0]
            recreated_tree = subprocess.check_output([GIT, "merge-tree", parents[0], parents[1]]).decode('utf8').splitlines()[0]
            if current_tree != recreated_tree:
                print("Merge commit {} is not clean".format(current_commit), file=sys.stderr)
                subprocess.call([GIT, 'diff', recreated_tree, current_tree])
                sys.exit(1)

        prev_commit = current_commit
        current_commit = parents[0]

if __name__ == '__main__':
    main()
