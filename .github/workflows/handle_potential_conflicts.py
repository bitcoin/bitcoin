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
import os
import json
import uuid
import requests

# need to install via pip
try:
    import hjson
except ImportError:
    print("Error: hjson module not found. Please install it with: pip install hjson", file=sys.stderr)
    sys.exit(1)

def get_pr_json(pr_num):
    try:
        response = requests.get(f'https://api.github.com/repos/dashpay/dash/pulls/{pr_num}')
        response.raise_for_status()
        pr_data = response.json()

        # Check if we got an error response
        if 'message' in pr_data and 'head' not in pr_data:
            print(f"Warning: GitHub API error for PR {pr_num}: {pr_data.get('message', 'Unknown error')}", file=sys.stderr)
            return None

        return pr_data
    except requests.RequestException as e:
        print(f"Warning: Error fetching PR {pr_num}: {e}", file=sys.stderr)
        return None
    except json.JSONDecodeError as e:
        print(f"Warning: Error parsing JSON for PR {pr_num}: {e}", file=sys.stderr)
        return None

def set_github_output(name, value):
    """Set GitHub Actions output"""
    if 'GITHUB_OUTPUT' not in os.environ:
        print(f"Warning: GITHUB_OUTPUT not set, skipping output: {name}={value}", file=sys.stderr)
        return

    try:
        with open(os.environ['GITHUB_OUTPUT'], 'a') as f:
            # For multiline values, use the delimiter syntax
            if '\n' in str(value):
                delimiter = f"EOF_{uuid.uuid4()}"
                f.write(f"{name}<<{delimiter}\n{value}\n{delimiter}\n")
            else:
                f.write(f"{name}={value}\n")
    except IOError as e:
        print(f"Error writing to GITHUB_OUTPUT: {e}", file=sys.stderr)

def main():
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <conflicts>', file=sys.stderr)
        sys.exit(1)

    conflict_input = sys.argv[1]
    print(f"Debug: Input received: {conflict_input}", file=sys.stderr)

    try:
        j_input = hjson.loads(conflict_input)
    except Exception as e:
        print(f"Error parsing input JSON: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Debug: Parsed input: {j_input}", file=sys.stderr)

    # Validate required fields
    if 'pull_number' not in j_input:
        print("Error: 'pull_number' field missing from input", file=sys.stderr)
        sys.exit(1)
    if 'conflictPrs' not in j_input:
        print("Error: 'conflictPrs' field missing from input", file=sys.stderr)
        sys.exit(1)

    our_pr_num = j_input['pull_number']
    our_pr_json = get_pr_json(our_pr_num)

    if our_pr_json is None:
        print(f"Error: Failed to fetch PR {our_pr_num}", file=sys.stderr)
        sys.exit(1)

    if 'head' not in our_pr_json or 'label' not in our_pr_json['head']:
        print(f"Error: Invalid PR data structure for PR {our_pr_num}", file=sys.stderr)
        sys.exit(1)

    our_pr_label = our_pr_json['head']['label']
    conflict_prs = j_input['conflictPrs']

    good = []
    bad = []
    conflict_details = []

    for conflict in conflict_prs:
        if 'number' not in conflict:
            print("Warning: Skipping conflict entry without 'number' field", file=sys.stderr)
            continue

        conflict_pr_num = conflict['number']
        print(f"Debug: Checking PR #{conflict_pr_num}", file=sys.stderr)

        conflict_pr_json = get_pr_json(conflict_pr_num)

        if conflict_pr_json is None:
            print(f"Warning: Failed to fetch PR {conflict_pr_num}, skipping", file=sys.stderr)
            continue

        if 'head' not in conflict_pr_json or 'label' not in conflict_pr_json['head']:
            print(f"Warning: Invalid PR data structure for PR {conflict_pr_num}, skipping", file=sys.stderr)
            continue

        conflict_pr_label = conflict_pr_json['head']['label']
        print(f"Debug: PR #{conflict_pr_num} label: {conflict_pr_label}", file=sys.stderr)

        if conflict_pr_json.get('mergeable_state') == "dirty":
            print(f'PR #{conflict_pr_num} needs rebase. Skipping conflict check', file=sys.stderr)
            continue

        if conflict_pr_json.get('draft', False):
            print(f'PR #{conflict_pr_num} is a draft. Skipping conflict check', file=sys.stderr)
            continue

        try:
            pre_mergeable = requests.get(f'https://github.com/dashpay/dash/branches/pre_mergeable/{our_pr_label}...{conflict_pr_label}')
            pre_mergeable.raise_for_status()
        except requests.RequestException as e:
            print(f"Error checking mergeability for PR {conflict_pr_num}: {e}", file=sys.stderr)
            continue

        if "These branches can be automatically merged." in pre_mergeable.text:
            good.append(conflict_pr_num)
        elif "Can't automatically merge" in pre_mergeable.text:
            bad.append(conflict_pr_num)
            conflict_details.append({
                'number': conflict_pr_num,
                'title': conflict_pr_json.get('title', 'Unknown'),
                'url': conflict_pr_json.get('html_url', f'https://github.com/dashpay/dash/pull/{conflict_pr_num}')
            })
        else:
            print(f"Warning: Unexpected response for PR {conflict_pr_num} mergeability check. Response snippet: {pre_mergeable.text[:200]}", file=sys.stderr)

    print(f"Not conflicting PRs: {good}", file=sys.stderr)
    print(f"Conflicting PRs: {bad}", file=sys.stderr)

    # Set GitHub Actions outputs
    if 'GITHUB_OUTPUT' in os.environ:
        set_github_output('has_conflicts', 'true' if len(bad) > 0 else 'false')

        # Format conflict details as markdown list
        if conflict_details:
            markdown_list = []
            for conflict in conflict_details:
                markdown_list.append(f"- #{conflict['number']} - [{conflict['title']}]({conflict['url']})")
            conflict_markdown = '\n'.join(markdown_list)
            set_github_output('conflict_details', conflict_markdown)
        else:
            set_github_output('conflict_details', '')

        set_github_output('conflicting_prs', ','.join(map(str, bad)))

    if len(bad) > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
