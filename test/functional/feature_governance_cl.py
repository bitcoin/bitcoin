#!/usr/bin/env python3
# Copyright (c) 2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests governance checks can be skipped for blocks covered by the best chainlock."""

import json

from test_framework.messages import uint256_to_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, force_finish_mnsync, satoshi_round

class DashGovernanceTest (DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, [["-budgetparams=10:10:10"]] * 6, fast_dip3_enforcement=True)

    def prepare_object(self, object_type, parent_hash, creation_time, revision, name, amount, payment_address):
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
        collateral_hash = self.nodes[0].gobject("prepare", parent_hash, proposal_rev, proposal_time, proposal_hex)
        return {
            "parentHash": parent_hash,
            "collateralHash": collateral_hash,
            "createdAt": proposal_time,
            "revision": proposal_rev,
            "hex": proposal_hex,
            "data": proposal_template,
        }

    def have_trigger_for_height(self, sb_block_height, nodes = None):
        if nodes is None:
            nodes = self.nodes
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

    def run_test(self):
        sb_cycle = 20

        self.log.info("Make sure ChainLocks are active")

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()
        self.activate_v19(expected_activation_height=900)
        self.log.info("Activated v19 at height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        self.mine_cycle_quorum(llmq_type_name='llmq_test_dip0024', llmq_type=103)

        self.sync_blocks()
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        self.nodes[0].sporkupdate("SPORK_9_SUPERBLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Prepare and submit proposals")

        proposal_time = self.mocktime
        self.p0_payout_address = self.nodes[0].getnewaddress()
        self.p1_payout_address = self.nodes[0].getnewaddress()
        self.p0_amount = satoshi_round("1.1")
        self.p1_amount = satoshi_round("3.3")

        p0_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_0", self.p0_amount, self.p0_payout_address)
        p1_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_1", self.p1_amount, self.p1_payout_address)
        self.bump_mocktime(60 * 10 + 1)

        self.nodes[0].generate(6)
        self.bump_mocktime(6 * 156)
        self.sync_blocks()

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 2)
        assert_equal(len(self.nodes[0].gobject("list")), 0)

        self.p0_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p0_collateral_prepare["hex"], p0_collateral_prepare["collateralHash"])
        self.p1_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p1_collateral_prepare["hex"], p1_collateral_prepare["collateralHash"])

        assert_equal(len(self.nodes[0].gobject("list")), 2)

        self.log.info("Isolate a node so that it would miss votes and triggers")
        # Isolate a node
        self.isolate_node(5)

        self.log.info("Vote proposals")

        self.nodes[0].gobject("vote-many", self.p0_hash, "funding", "yes")
        self.nodes[0].gobject("vote-many", self.p1_hash, "funding", "yes")
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["YesCount"], self.mn_count)
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["YesCount"], self.mn_count)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)

        n = sb_cycle - self.nodes[0].getblockcount() % sb_cycle
        assert n > 1

        # Move remaining n blocks until the next Superblock
        for _ in range(n - 1):
            self.nodes[0].generate(1)
            self.bump_mocktime(156)
            self.sync_blocks(self.nodes[0:5])

        self.log.info("Wait for new trigger and votes on non-isolated nodes")
        sb_block_height = self.nodes[0].getblockcount() + 1
        self.wait_until(lambda: self.have_trigger_for_height(sb_block_height, self.nodes[0:5]), timeout=5)
        # Mine superblock
        self.nodes[0].generate(1)
        self.bump_mocktime(156)
        self.sync_blocks(self.nodes[0:5])
        self.wait_for_chainlocked_block(self.nodes[0], self.nodes[0].getbestblockhash())

        self.log.info("Reconnect isolated node and confirm the next ChainLock will let it sync")
        self.reconnect_isolated_node(5, 0)
        # Force isolated node to be fully synced so that it would not request gov objects when reconnected
        assert_equal(self.nodes[5].mnsync("status")["IsSynced"], False)
        force_finish_mnsync(self.nodes[5])
        self.nodes[0].generate(1)
        self.bump_mocktime(156)
        self.sync_blocks()


if __name__ == '__main__':
    DashGovernanceTest().main()
