#!/usr/bin/env python3
# Copyright (c) 2022-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""

Usage:
    $ ./handle_potential_conflicts.py <conflicts>

Where <conflicts> is a json string which looks like
    {   pull_number: 26,
        conflictPrs:
            [ { number: 15,
                files: [ 'testfile1', `testfile2` ],
                conflicts: [ 'testfile1' ] },
                ...
            ]}
"""

import sys
import requests

# need to install via pip
import hjson

def get_label(pr_num):
    return requests.get(f'https://api.github.com/repos/dashpay/dash/pulls/{pr_num}').json()['head']['label']

def main():
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <conflicts>', file=sys.stderr)
        sys.exit(1)

    input = sys.argv[1]
    print(input)
    j_input = hjson.loads(input)
    print(j_input)


    our_pr_num = j_input['pull_number']
    our_pr_label = get_label(our_pr_num)
    conflictPrs = j_input['conflictPrs']

    good = []
    bad = []

    for conflict in conflictPrs:
        this_pr_num = conflict['number']
        print(this_pr_num)

        r = requests.get(f'https://api.github.com/repos/dashpay/dash/pulls/{this_pr_num}')
        print(r.json()['head']['label'])

        mergable_state = r.json()['mergeable_state']
        if mergable_state == "dirty":
            print(f'{this_pr_num} needs rebase. Skipping conflict check')
            continue

        r = requests.get(f'https://github.com/dashpay/dash/branches/pre_mergeable/{our_pr_label}...{get_label(this_pr_num)}')
        if "These branches can be automatically merged." in r.text:
            good.append(this_pr_num)
        elif "Can’t automatically merge" in r.text:
            bad.append(this_pr_num)
        else:
            raise Exception("not mergeable or unmergable!")

    print("Not conflicting PRs: ", good)

    print("Conflicting PRs: ", bad)
    if len(bad) > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
