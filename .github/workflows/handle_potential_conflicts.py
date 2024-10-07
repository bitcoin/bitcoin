#!/usr/bin/env python3
# Copyright (c) 2022-2024 The Dash Core developers
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

def get_pr_json(pr_num):
    return requests.get(f'https://api.github.com/repos/dashpay/dash/pulls/{pr_num}').json()

def main():
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <conflicts>', file=sys.stderr)
        sys.exit(1)

    input = sys.argv[1]
    print(input)
    j_input = hjson.loads(input)
    print(j_input)


    our_pr_num = j_input['pull_number']
    our_pr_label = get_pr_json(our_pr_num)['head']['label']
    conflictPrs = j_input['conflictPrs']

    good = []
    bad = []

    for conflict in conflictPrs:
        conflict_pr_num = conflict['number']
        print(conflict_pr_num)

        conflict_pr_json = get_pr_json(conflict_pr_num)
        conflict_pr_label = conflict_pr_json['head']['label']
        print(conflict_pr_label)

        if conflict_pr_json['mergeable_state'] == "dirty":
            print(f'{conflict_pr_num} needs rebase. Skipping conflict check')
            continue

        if conflict_pr_json['draft']:
            print(f'{conflict_pr_num} is a draft. Skipping conflict check')
            continue

        pre_mergeable = requests.get(f'https://github.com/dashpay/dash/branches/pre_mergeable/{our_pr_label}...{conflict_pr_label}')
        if "These branches can be automatically merged." in pre_mergeable.text:
            good.append(conflict_pr_num)
        elif "Can’t automatically merge" in pre_mergeable.text:
            bad.append(conflict_pr_num)
        else:
            raise Exception("not mergeable or unmergable!")

    print("Not conflicting PRs: ", good)

    print("Conflicting PRs: ", bad)
    if len(bad) > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
