#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests around dash governance."""

import json

from test_framework.messages import uint256_to_string
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, satoshi_round, set_node_times, wait_until

class DashGovernanceTest (DashTestFramework):
    def set_test_params(self):
        self.v20_start_time = 1417713500
        # using adjusted v20 deployment params to test an edge case where superblock maturity window is equal to deployment window size
        self.set_dash_test_params(6, 5, [["-budgetparams=10:10:10", f"-vbparams=v20:{self.v20_start_time}:999999999999:10:8:6:5:0"]] * 6, fast_dip3_enforcement=True)

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

    def check_superblockbudget(self, v20_active):
        v20_state = self.nodes[0].getblockchaininfo()["softforks"]["v20"]
        assert_equal(v20_state["active"], v20_active)
        assert_equal(self.nodes[0].getsuperblockbudget(200), self.expected_old_budget)
        assert_equal(self.nodes[0].getsuperblockbudget(220), self.expected_old_budget)
        if v20_state["bip9"]["status"] == "locked_in" or v20_state["bip9"]["status"] == "active":
            assert_equal(self.nodes[0].getsuperblockbudget(240), self.expected_v20_budget)
            assert_equal(self.nodes[0].getsuperblockbudget(260), self.expected_v20_budget)
            assert_equal(self.nodes[0].getsuperblockbudget(280), self.expected_v20_budget)
        else:
            assert_equal(self.nodes[0].getsuperblockbudget(240), self.expected_old_budget)
            assert_equal(self.nodes[0].getsuperblockbudget(260), self.expected_old_budget)
            assert_equal(self.nodes[0].getsuperblockbudget(280), self.expected_old_budget)

    def check_superblock(self):
        # Make sure Superblock has only payments that fit into the budget
        # p0 must always be included because it has most votes
        # p1 and p2 have equal number of votes (but less votes than p0)
        # so only one of them can be included (depends on proposal hashes).

        coinbase_outputs = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]["vout"]
        payments_found = 0
        for txout in coinbase_outputs:
            if txout["value"] == self.p0_amount and txout["scriptPubKey"]["addresses"][0] == self.p0_payout_address:
                payments_found += 1
            if txout["value"] == self.p1_amount and txout["scriptPubKey"]["addresses"][0] == self.p1_payout_address:
                if self.p1_hash > self.p2_hash:
                    payments_found += 1
                else:
                    assert False
            if txout["value"] == self.p2_amount and txout["scriptPubKey"]["addresses"][0] == self.p2_payout_address:
                if self.p2_hash > self.p1_hash:
                    payments_found += 1
                else:
                    assert False

        assert_equal(payments_found, 2)

    def run_test(self):
        map_vote_outcomes = {
            0: "none",
            1: "yes",
            2: "no",
            3: "abstain"
        }
        map_vote_signals = {
            0: "none",
            1: "funding",
            2: "valid",
            3: "delete",
            4: "endorsed"
        }
        sb_cycle = 20
        sb_maturity_window = 10
        sb_immaturity_window = sb_cycle - sb_maturity_window
        self.expected_old_budget = satoshi_round("928.57142840")
        self.expected_v20_budget = satoshi_round("18.57142860")

        self.activate_dip8()

        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 4070908800)
        self.nodes[0].sporkupdate("SPORK_9_SUPERBLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 0)

        self.nodes[0].generate(3)
        self.bump_mocktime(3)
        self.sync_blocks()
        assert_equal(self.nodes[0].getblockcount(), 210)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["bip9"]["status"], "defined")
        self.check_superblockbudget(False)

        assert self.mocktime < self.v20_start_time
        self.mocktime = self.v20_start_time
        set_node_times(self.nodes, self.mocktime)

        self.nodes[0].generate(10)
        self.bump_mocktime(10)
        self.sync_blocks()
        assert_equal(self.nodes[0].getblockcount(), 220)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["bip9"]["status"], "started")
        self.check_superblockbudget(False)

        proposal_time = self.mocktime
        self.p0_payout_address = self.nodes[0].getnewaddress()
        self.p1_payout_address = self.nodes[0].getnewaddress()
        self.p2_payout_address = self.nodes[0].getnewaddress()
        self.p0_amount = satoshi_round("1.1")
        self.p1_amount = satoshi_round("3.3")
        self.p2_amount = self.expected_v20_budget - self.p1_amount

        p0_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_0", self.p0_amount, self.p0_payout_address)
        p1_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_1", self.p1_amount, self.p1_payout_address)
        p2_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_2", self.p2_amount, self.p2_payout_address)

        self.nodes[0].generate(6)
        self.bump_mocktime(6)
        self.sync_blocks()

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 3)
        assert_equal(len(self.nodes[0].gobject("list")), 0)

        self.p0_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p0_collateral_prepare["hex"], p0_collateral_prepare["collateralHash"])
        self.p1_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p1_collateral_prepare["hex"], p1_collateral_prepare["collateralHash"])
        self.p2_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p2_collateral_prepare["hex"], p2_collateral_prepare["collateralHash"])

        assert_equal(len(self.nodes[0].gobject("list")), 3)

        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["NoCount"], 0)

        self.nodes[0].gobject("vote-alias", self.p0_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-many", self.p0_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["YesCount"], self.mn_count - 1)
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["NoCount"], 1)

        self.nodes[0].gobject("vote-alias", self.p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-alias", self.p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject("vote-many", self.p1_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["NoCount"], 2)

        self.nodes[0].gobject("vote-alias", self.p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-alias", self.p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject("vote-many", self.p2_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["NoCount"], 2)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)

        block_count = self.nodes[0].getblockcount()

        # Move until 1 block before the Superblock maturity window starts
        n = sb_immaturity_window - block_count % sb_cycle
        # v20 is expected to be activate since block 240
        assert block_count + n < 240
        for _ in range(n - 1):
            self.nodes[0].generate(1)
            self.bump_mocktime(1)
            self.sync_blocks()
            self.check_superblockbudget(False)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)

        # Detect payee node
        mn_list = self.nodes[0].protx("list", "registered", True)
        height_protx_list = []
        for mn in mn_list:
            height_protx_list.append((mn['state']['lastPaidHeight'], mn['proTxHash']))

        height_protx_list = sorted(height_protx_list)
        _, mn_payee_protx = height_protx_list[1]

        payee_idx = None
        for mn in self.mninfo:
            if mn.proTxHash == mn_payee_protx:
                payee_idx = mn.nodeIdx
                break
        assert payee_idx is not None

        # Isolate payee node and create a trigger
        self.isolate_node(payee_idx)
        isolated = self.nodes[payee_idx]

        # Move 1 block inside the Superblock maturity window on the isolated node
        isolated.generate(1)
        self.bump_mocktime(1)
        # The isolated "winner" should submit new trigger and vote for it
        wait_until(lambda: len(isolated.gobject("list", "valid", "triggers")) == 1, timeout=5)
        isolated_trigger_hash = list(isolated.gobject("list", "valid", "triggers").keys())[0]
        wait_until(lambda: list(isolated.gobject("list", "valid", "triggers").values())[0]['YesCount'] == 1, timeout=5)
        more_votes = wait_until(lambda: list(isolated.gobject("list", "valid", "triggers").values())[0]['YesCount'] > 1, timeout=5, do_assert=False)
        assert_equal(more_votes, False)

        # Move 1 block enabling the Superblock maturity window on non-isolated nodes
        self.nodes[0].generate(1)
        self.bump_mocktime(1)
        assert_equal(self.nodes[0].getblockcount(), 230)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["bip9"]["status"], "locked_in")
        self.check_superblockbudget(False)

        # The "winner" should submit new trigger and vote for it, but it's isolated so no triggers should be found
        has_trigger = wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) >= 1, timeout=5, do_assert=False)
        assert_equal(has_trigger, False)

        # Move 1 block inside the Superblock maturity window on non-isolated nodes
        self.nodes[0].generate(1)
        self.bump_mocktime(1)

        # There is now new "winner" who should submit new trigger and vote for it
        wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) == 1, timeout=5)
        winning_trigger_hash = list(self.nodes[0].gobject("list", "valid", "triggers").keys())[0]
        wait_until(lambda: list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]['YesCount'] == 1, timeout=5)
        more_votes = wait_until(lambda: list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]['YesCount'] > 1, timeout=5, do_assert=False)
        assert_equal(more_votes, False)

        # Make sure amounts aren't trimmed
        payment_amounts_expected = [str(satoshi_round(str(self.p0_amount))), str(satoshi_round(str(self.p1_amount))), str(satoshi_round(str(self.p2_amount)))]
        data_string = list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]["DataString"]
        payment_amounts_trigger = json.loads(data_string)["payment_amounts"].split("|")
        for amount_str in payment_amounts_trigger:
            assert(amount_str in payment_amounts_expected)

        # Move another block inside the Superblock maturity window on non-isolated nodes
        self.nodes[0].generate(1)
        self.bump_mocktime(1)

        # Every non-isolated MN should vote for the same trigger now, no new triggers should be created
        wait_until(lambda: list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]['YesCount'] == self.mn_count - 1, timeout=5)
        more_triggers = wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) > 1, timeout=5, do_assert=False)
        assert_equal(more_triggers, False)

        self.reconnect_isolated_node(payee_idx, 0)
        # self.connect_nodes(0, payee_idx)
        self.sync_blocks()

        # re-sync helper
        def sync_gov(node):
            self.bump_mocktime(1)
            return node.mnsync("status")["IsSynced"]

        for node in self.nodes:
            # Force sync
            node.mnsync("reset")
            # fast-forward to governance sync
            node.mnsync("next")
            wait_until(lambda: sync_gov(node))

        # Should see two triggers now
        wait_until(lambda: len(isolated.gobject("list", "valid", "triggers")) == 2, timeout=5)
        wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) == 2, timeout=5)
        more_triggers = wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) > 2, timeout=5, do_assert=False)
        assert_equal(more_triggers, False)

        # Move another block inside the Superblock maturity window
        self.nodes[0].generate(1)
        self.bump_mocktime(1)
        self.sync_blocks()

        # Should see NO votes on both triggers now
        wait_until(lambda: self.nodes[0].gobject("list", "valid", "triggers")[winning_trigger_hash]['NoCount'] == 1, timeout=5)
        wait_until(lambda: self.nodes[0].gobject("list", "valid", "triggers")[isolated_trigger_hash]['NoCount'] == self.mn_count - 1, timeout=5)

        block_count = self.nodes[0].getblockcount()
        n = sb_cycle - block_count % sb_cycle

        # Move remaining n blocks until actual Superblock
        for i in range(n):
            self.nodes[0].generate(1)
            self.bump_mocktime(1)
            self.sync_blocks()
            # comparing to 239 because bip9 forks are active when the tip is one block behind the activation height
            self.check_superblockbudget(block_count + i + 1 >= 239)

        self.check_superblockbudget(True)
        self.check_superblock()

        # Mine and check a couple more superblocks
        for i in range(2):
            for _ in range(20):
                self.nodes[0].generate(1)
                self.bump_mocktime(1)
                self.sync_blocks()
            assert_equal(self.nodes[0].getblockcount(), 240 + (i + 1) * 20)
            assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["bip9"]["status"], "active")
            self.check_superblockbudget(True)
            self.check_superblock()


if __name__ == '__main__':
    DashGovernanceTest().main()
