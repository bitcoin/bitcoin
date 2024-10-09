#!/usr/bin/env python3
# Copyright (c) 2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests governance checks can be skipped for blocks covered by the best chainlock."""

import json
import time

from test_framework.governance import have_trigger_for_height
from test_framework.messages import uint256_to_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, satoshi_round

class DashGovernanceTest (DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, [["-budgetparams=10:10:10"]] * 6)

    def prepare_object(self, object_type, parent_hash, creation_time, revision, name, amount, payment_address):
        proposal_rev = revision
        proposal_time = int(creation_time)
        proposal_template = {
            "type": object_type,
            "name": name,
            "start_epoch": proposal_time,
            "end_epoch": proposal_time + 20 * 156,
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

    def run_test(self):
        sb_cycle = 20
        sb_maturity_window = 10
        sb_immaturity_window = sb_cycle - sb_maturity_window

        self.log.info("Make sure ChainLocks are active")

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()
        self.mine_cycle_quorum()

        self.sync_blocks()
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        self.nodes[0].sporkupdate("SPORK_9_SUPERBLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        # Move to the superblock cycle start block
        n = sb_cycle - self.nodes[0].getblockcount() % sb_cycle
        for _ in range(n):
            self.bump_mocktime(156)
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks())

        self.log.info("Prepare proposals")

        proposal_time = self.mocktime
        self.p0_payout_address = self.nodes[0].getnewaddress()
        self.p1_payout_address = self.nodes[0].getnewaddress()
        self.p0_amount = satoshi_round("1.1")
        self.p1_amount = satoshi_round("3.3")

        p0_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_0", self.p0_amount, self.p0_payout_address)
        p1_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_1", self.p1_amount, self.p1_payout_address)
        self.bump_mocktime(60 * 10 + 1)

        self.generate(self.nodes[0], 6, sync_fun=self.sync_blocks())

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 2)
        assert_equal(len(self.nodes[0].gobject("list")), 0)

        self.log.info("Submit proposals")

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

        self.log.info("Move 1 block into sb maturity window")
        n = sb_immaturity_window - self.nodes[0].getblockcount() % sb_cycle
        assert n >= 0
        for _ in range(n + 1):
            self.bump_mocktime(156)
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(self.nodes[0:5]))

        self.log.info("Wait for new trigger and votes on non-isolated nodes")
        sb_block_height = self.nodes[0].getblockcount() // sb_cycle * sb_cycle + sb_cycle
        assert_equal(sb_block_height % sb_cycle, 0)
        self.wait_until(lambda: have_trigger_for_height(self.nodes[0:5], sb_block_height), timeout=5)

        n = sb_cycle - self.nodes[0].getblockcount() % sb_cycle
        assert n > 1

        self.log.info("Move remaining n blocks until the next Superblock")
        for _ in range(n - 1):
            self.bump_mocktime(156)
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(self.nodes[0:5]))

        # Confirm all is good
        self.wait_until(lambda: have_trigger_for_height(self.nodes[0:5], sb_block_height), timeout=5)

        self.log.info("Mine superblock")
        self.bump_mocktime(156)
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(self.nodes[0:5]))
        self.wait_for_chainlocked_block(self.nodes[0], self.nodes[0].getbestblockhash())

        self.log.info("Mine (superblock cycle + 1) blocks on non-isolated nodes to forget about this trigger")
        for _ in range(sb_cycle):
            self.bump_mocktime(156)
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(self.nodes[0:5]))
        # Should still have at least 1 trigger for the old sb cycle and 0 for the current one
        assert len(self.nodes[0].gobject("list", "valid", "triggers")) >= 1
        assert not have_trigger_for_height(self.nodes[0:5], sb_block_height + sb_cycle)
        self.bump_mocktime(156)
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(self.nodes[0:5]))
        # Trigger scheduler to mark old triggers for deletion
        self.bump_mocktime(5 * 60)
        # Let it do the job
        time.sleep(1)
        # Move forward to satisfy GOVERNANCE_DELETION_DELAY, should actually remove old triggers now
        self.bump_mocktime(10 * 60)
        self.wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) == 0, timeout=5)

        self.log.info("Reconnect isolated node and confirm the next ChainLock will let it sync")
        self.reconnect_isolated_node(5, 0)
        assert_equal(self.nodes[5].mnsync("status")["IsSynced"], False)
        # NOTE: bumping mocktime too much after recent reconnect can result in "timeout downloading block"
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())


if __name__ == '__main__':
    DashGovernanceTest().main()
