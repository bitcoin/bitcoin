#!/usr/bin/env python3
# Copyright (c) 2018-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Utils for dash governance tests."""

import json

EXPECTED_STDERR_NO_GOV = "Warning: You are starting with governance validation disabled."
EXPECTED_STDERR_NO_GOV_PRUNE = f"{EXPECTED_STDERR_NO_GOV} This is expected because you are running a pruned node."

def prepare_object(node, object_type, parent_hash, creation_time, revision, name, amount, payment_address):
    proposal_rev = revision
    proposal_time = int(creation_time)
    proposal_template = {
        "type": object_type,
        "name": name,
        "start_epoch": proposal_time,
        "end_epoch": proposal_time + 24 * 60 * 60,
        "payment_amount": float(amount),
        "payment_address": payment_address,
        "url": "https://dash.org"
    }
    proposal_hex = ''.join(format(x, '02x') for x in json.dumps(proposal_template).encode())
    collateral_hash = node.gobject("prepare", parent_hash, proposal_rev, proposal_time, proposal_hex)
    return {
        "parentHash": parent_hash,
        "collateralHash": collateral_hash,
        "createdAt": proposal_time,
        "revision": proposal_rev,
        "hex": proposal_hex,
        "data": proposal_template,
    }

def have_trigger_for_height(nodes, sb_block_height):
    count = 0
    for node in nodes:
        valid_triggers = node.gobject("list", "valid", "triggers")
        for trigger in list(valid_triggers.values()):
            if json.loads(trigger["DataString"])["event_block_height"] != sb_block_height:
                continue
            if trigger['AbsoluteYesCount'] > 0:
                count = count + 1
                break
    return count == len(nodes)

