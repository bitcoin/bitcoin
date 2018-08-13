#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests around dash governance objects."""

import json
import time

from test_framework.messages import uint256_to_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, assert_greater_than, assert_raises_rpc_error


def validate_object(prepared, rpc_prepared):
    assert_equal(prepared["parentHash"], rpc_prepared["parentHash"])
    assert_equal(prepared["collateralHash"], rpc_prepared["collateralHash"])
    assert_equal(prepared["createdAt"], rpc_prepared["createdAt"])
    assert_equal(prepared["revision"], rpc_prepared["revision"])
    assert_equal(prepared["hex"], rpc_prepared["data"]["hex"])
    del rpc_prepared["data"]["hex"]
    assert_equal(prepared["data"], rpc_prepared["data"])


class DashGovernanceTest (DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(2, 1)

    def prepare_object(self, object_type, parent_hash, creation_time, revision, name, amount):
        proposal_rev = revision
        proposal_time = int(creation_time)
        proposal_template = {
            "type": object_type,
            "name": name,
            "start_epoch": proposal_time,
            "end_epoch": proposal_time + 24 * 60 * 60,
            "payment_amount": amount,
            "payment_address": self.nodes[0].getnewaddress(),
            "url": "https://dash.org"
        }
        proposal_hex = ''.join(format(x, '02x') for x in json.dumps(proposal_template).encode())
        collateral_hash = self.nodes[0].gobject("prepare", parent_hash, proposal_rev, proposal_time, proposal_hex)
        return {
            "parentHash": parent_hash,
            "collateralHash": collateral_hash,
            "createdAt": proposal_time,
            "revision": proposal_rev,
            "hex": proposal_hex,
            "data": proposal_template,
        }

    def run_test(self):

        time_start = time.time()
        object_type = 1  # GOVERNANCE PROPOSAL

        # At start there should be no prepared objects available
        assert_equal(len(self.nodes[0].gobject("list-prepared")), 0)
        # Create 5 proposals with different creation times and validate their ordered like expected
        p1 = self.prepare_object(object_type, uint256_to_string(0), time_start, 0, "SortByTime1", 1)
        p2 = self.prepare_object(object_type, uint256_to_string(0), time_start + 10, 1000, "SortByTime2", 10)
        p3 = self.prepare_object(object_type, uint256_to_string(0), time_start - 10, 1000000000, "SortByTime3", 20)
        p4 = self.prepare_object(object_type, uint256_to_string(0), time_start + 1, -20, "SortByTime4", 400)
        p5 = self.prepare_object(object_type, uint256_to_string(0), time_start + 30, 1, "SortByTime5", 100000000)

        rpc_list_prepared = self.nodes[0].gobject("list-prepared")
        assert_equal(len(rpc_list_prepared), 5)

        expected_order = [p3, p1, p4, p2, p5]
        for i in range(len(expected_order)):
            validate_object(expected_order[i], rpc_list_prepared[i])

        # Create two more with the same time
        self.prepare_object(object_type, uint256_to_string(0), time_start + 60, 1, "SameTime1", 2)
        self.prepare_object(object_type, uint256_to_string(0), time_start + 60, 2, "SameTime2", 2)
        # Query them with count=2
        rpc_list_prepared = self.nodes[0].gobject("list-prepared", 2)
        # And make sure it does only return 2 of the 7 available
        assert_equal(len(rpc_list_prepared), 2)
        # Since they have the same time they should be sorted by hex data, in this case, the second should be greater
        assert_greater_than(rpc_list_prepared[1]["data"]["hex"], rpc_list_prepared[0]["data"]["hex"])
        # Restart node0 and make sure it still contains all valid proposals after restart
        rpc_full_list_pre_restart = self.nodes[0].gobject("list-prepared")
        self.restart_node(0)
        rpc_full_list_post_restart = self.nodes[0].gobject("list-prepared")
        assert_equal(rpc_full_list_pre_restart, rpc_full_list_post_restart)
        # Create more objects so that we have a total of 11
        self.prepare_object(object_type, uint256_to_string(0), time_start, 0, "More1", 1)
        self.prepare_object(object_type, uint256_to_string(0), time_start, 0, "More2", 1)
        self.prepare_object(object_type, uint256_to_string(0), time_start, 0, "More3", 1)
        self.prepare_object(object_type, uint256_to_string(0), time_start, 0, "More4", 1)
        # Make sure default count is 10 while there are 11 in total
        assert_equal(len(self.nodes[0].gobject("list-prepared")), 10)
        assert_equal(len(self.nodes[0].gobject("list-prepared", 12)), 11)
        # Make sure it returns 0 objects with count=0
        assert_equal(len(self.nodes[0].gobject("list-prepared", 0)), 0)
        # And test some invalid count values
        assert_raises_rpc_error(-8, "Negative count", self.nodes[0].gobject, "list-prepared", -1)
        assert_raises_rpc_error(-8, "Negative count", self.nodes[0].gobject, "list-prepared", -1000)


if __name__ == '__main__':
    DashGovernanceTest().main()
