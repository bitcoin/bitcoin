#!/usr/bin/env python3
# Copyright (c) 2018-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests around dash governance."""

import json

from test_framework.messages import uint256_to_string
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.governance import have_trigger_for_height, prepare_object
from test_framework.util import assert_equal, satoshi_round

GOVERNANCE_UPDATE_MIN = 60 * 60 # src/governance/object.h

class DashGovernanceTest (DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, [[
            "-budgetparams=10:10:10",
        ]] * 6)
        self.delay_v20_and_mn_rr(height=160)

    def check_superblockbudget(self, v20_active):
        v20_state = self.nodes[0].getblockchaininfo()["softforks"]["v20"]
        assert_equal(v20_state["active"], v20_active)
        assert_equal(self.nodes[0].getsuperblockbudget(120), self.expected_old_budget)
        assert_equal(self.nodes[0].getsuperblockbudget(140), self.expected_old_budget)
        assert_equal(self.nodes[0].getsuperblockbudget(160), self.expected_v20_budget)
        assert_equal(self.nodes[0].getsuperblockbudget(180), self.expected_v20_budget)

    def check_superblock(self):
        # Make sure Superblock has only payments that fit into the budget
        # p0 must always be included because it has most votes
        # p1 and p2 have equal number of votes (but less votes than p0)
        # so only one of them can be included (depends on proposal hashes).

        coinbase_outputs = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]["vout"]
        payments_found = 0
        for txout in coinbase_outputs:
            if txout["value"] == self.p0_amount and txout["scriptPubKey"]["address"] == self.p0_payout_address:
                payments_found += 1
            if txout["value"] == self.p1_amount and txout["scriptPubKey"]["address"] == self.p1_payout_address:
                if self.p1_hash > self.p2_hash:
                    payments_found += 1
                else:
                    assert False
            if txout["value"] == self.p2_amount and txout["scriptPubKey"]["address"] == self.p2_payout_address:
                if self.p2_hash > self.p1_hash:
                    payments_found += 1
                else:
                    assert False

        assert_equal(payments_found, 2)

    def run_test(self):
        self.log.info("Start testing...")
        governance_info = self.nodes[0].getgovernanceinfo()
        assert_equal(governance_info['governanceminquorum'], 1)
        assert_equal(governance_info['proposalfee'], 1)
        assert_equal(governance_info['superblockcycle'], 20)
        assert_equal(governance_info['superblockmaturitywindow'], 10)
        assert_equal(governance_info['lastsuperblock'], 120)
        assert_equal(governance_info['nextsuperblock'], governance_info['lastsuperblock'] + governance_info['superblockcycle'])
        assert_equal(governance_info['governancebudget'], 1000)

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
        self.expected_old_budget = satoshi_round("1000")
        self.expected_v20_budget = satoshi_round("18.57142860")

        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 4070908800)
        self.nodes[0].sporkupdate("SPORK_9_SUPERBLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 0)

        self.log.info("Check 1st superblock before v20")
        self.bump_mocktime(3)
        self.generate(self.nodes[0], 3, sync_fun=self.sync_blocks())
        assert_equal(self.nodes[0].getblockcount(), 137)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["active"], False)
        self.check_superblockbudget(False)

        self.log.info("Check 2nd superblock before v20")
        self.bump_mocktime(3)
        self.generate(self.nodes[0], 3, sync_fun=self.sync_blocks())
        assert_equal(self.nodes[0].getblockcount(), 140)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["active"], False)
        self.check_superblockbudget(False)

        self.log.info("Prepare proposals")
        proposal_time = self.mocktime
        self.p0_payout_address = self.nodes[0].getnewaddress()
        self.p1_payout_address = self.nodes[0].getnewaddress()
        self.p2_payout_address = self.nodes[0].getnewaddress()
        self.p0_amount = satoshi_round("1.1")
        self.p1_amount = satoshi_round("3.3")
        self.p2_amount = self.expected_v20_budget - self.p1_amount

        p0_collateral_prepare = prepare_object(self.nodes[0], 1, uint256_to_string(0), proposal_time, 1, "Proposal_0", self.p0_amount, self.p0_payout_address)
        p1_collateral_prepare = prepare_object(self.nodes[0], 1, uint256_to_string(0), proposal_time, 1, "Proposal_1", self.p1_amount, self.p1_payout_address)
        p2_collateral_prepare = prepare_object(self.nodes[0], 1, uint256_to_string(0), proposal_time, 1, "Proposal_2", self.p2_amount, self.p2_payout_address)

        self.bump_mocktime(6)
        self.generate(self.nodes[0], 6, sync_fun=self.sync_blocks())

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 3)
        assert_equal(len(self.nodes[0].gobject("list")), 0)

        self.log.info("Submit objects")
        self.p0_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p0_collateral_prepare["hex"], p0_collateral_prepare["collateralHash"])
        self.p1_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p1_collateral_prepare["hex"], p1_collateral_prepare["collateralHash"])
        self.p2_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p2_collateral_prepare["hex"], p2_collateral_prepare["collateralHash"])

        assert_equal(len(self.nodes[0].gobject("list")), 3)
        self.wait_until(lambda: len(self.nodes[1].gobject("list")) == 3, timeout = 5)

        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["NoCount"], 0)

        self.log.info("Cast votes")
        self.nodes[0].gobject("vote-alias", self.p0_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-many", self.p0_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["YesCount"], self.mn_count - 1)
        assert_equal(self.nodes[0].gobject("get", self.p0_hash)["FundingResult"]["NoCount"], 1)
        self.wait_until(lambda: self.nodes[1].gobject("get", self.p0_hash)["FundingResult"]["YesCount"] == self.mn_count - 1, timeout = 5)
        self.wait_until(lambda: self.nodes[1].gobject("get", self.p0_hash)["FundingResult"]["NoCount"] == 1, timeout = 5)

        self.nodes[0].gobject("vote-alias", self.p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-alias", self.p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject("vote-many", self.p1_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject("get", self.p1_hash)["FundingResult"]["NoCount"], 2)
        self.wait_until(lambda: self.nodes[1].gobject("get", self.p1_hash)["FundingResult"]["YesCount"] == self.mn_count - 2, timeout = 5)
        self.wait_until(lambda: self.nodes[1].gobject("get", self.p1_hash)["FundingResult"]["NoCount"] == 2, timeout = 5)

        self.nodes[0].gobject("vote-alias", self.p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-alias", self.p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject("vote-many", self.p2_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject("get", self.p2_hash)["FundingResult"]["NoCount"], 2)
        self.wait_until(lambda: self.nodes[1].gobject("get", self.p2_hash)["FundingResult"]["YesCount"] == self.mn_count - 2, timeout = 5)
        self.wait_until(lambda: self.nodes[1].gobject("get", self.p2_hash)["FundingResult"]["NoCount"] == 2, timeout = 5)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)
        assert_equal(self.nodes[0].gobject("count")["votes"], 15)
        assert_equal(self.nodes[1].gobject("count")["votes"], 15)

        block_count = self.nodes[0].getblockcount()

        self.log.info("Move until 1 block before the Superblock maturity window starts")
        n = sb_immaturity_window - block_count % sb_cycle
        self.log.info("v20 is expected to be activate since block 160")
        assert block_count + n < 160
        for _ in range(n - 1):
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
            self.check_superblockbudget(False)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)

        self.log.info("Detect payee node")
        mn_list = self.nodes[0].protx("list", "registered", True)
        height_protx_list = []
        for mn in mn_list:
            height_protx_list.append((mn['state']['lastPaidHeight'], mn['proTxHash']))

        height_protx_list = sorted(height_protx_list)
        _, mn_payee_protx = height_protx_list[1]

        payee_idx = None
        for mn in self.mninfo: # type: MasternodeInfo
            if mn.proTxHash == mn_payee_protx:
                payee_idx = mn.nodeIdx
                break
        assert payee_idx is not None

        self.log.info("Isolate payee node and create a trigger")
        self.isolate_node(payee_idx)
        isolated = self.nodes[payee_idx]

        self.log.info("Move 1 block inside the Superblock maturity window on the isolated node")
        self.bump_mocktime(1)
        self.generate(isolated, 1, sync_fun=self.no_op)
        self.log.info("The isolated 'winner' should submit new trigger and vote for it")
        self.wait_until(lambda: len(isolated.gobject("list", "valid", "triggers")) == 1, timeout=5)
        isolated_trigger_hash = list(isolated.gobject("list", "valid", "triggers").keys())[0]
        self.wait_until(lambda: list(isolated.gobject("list", "valid", "triggers").values())[0]['YesCount'] == 1, timeout=5)
        more_votes = self.wait_until(lambda: list(isolated.gobject("list", "valid", "triggers").values())[0]['YesCount'] > 1, timeout=5, do_assert=False)
        assert_equal(more_votes, False)
        assert_equal(self.nodes[0].gobject("count")["votes"], 15)
        assert_equal(isolated.gobject("count")["votes"], 16)

        self.log.info("Move 1 block enabling the Superblock maturity window on non-isolated nodes")
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        assert_equal(self.nodes[0].getblockcount(), 150)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["active"], False)
        self.check_superblockbudget(False)

        self.log.info("The 'winner' should submit new trigger and vote for it, but it's isolated so no triggers should be found")
        has_trigger = self.wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) >= 1, timeout=5, do_assert=False)
        assert_equal(has_trigger, False)

        self.log.info("Move 1 block inside the Superblock maturity window on non-isolated nodes")
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        self.log.info("There is now new 'winner' who should submit new trigger and vote for it")
        self.wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) == 1, timeout=5)
        winning_trigger_hash = list(self.nodes[0].gobject("list", "valid", "triggers").keys())[0]
        self.wait_until(lambda: list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]['YesCount'] == 1, timeout=5)
        more_votes = self.wait_until(lambda: list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]['YesCount'] > 1, timeout=5, do_assert=False)
        assert_equal(more_votes, False)
        assert_equal(self.nodes[0].gobject("count")["votes"], 16)
        assert_equal(isolated.gobject("count")["votes"], 16)

        self.log.info("Make sure amounts aren't trimmed")
        payment_amounts_expected = [str(satoshi_round(str(self.p0_amount))), str(satoshi_round(str(self.p1_amount))), str(satoshi_round(str(self.p2_amount)))]
        data_string = list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]["DataString"]
        payment_amounts_trigger = json.loads(data_string)["payment_amounts"].split("|")
        for amount_str in payment_amounts_trigger:
            assert(amount_str in payment_amounts_expected)

        self.log.info("Move another block inside the Superblock maturity window on non-isolated nodes")
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        self.log.info("Every non-isolated MN should vote for the same trigger now, no new triggers should be created")
        self.wait_until(lambda: list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]['YesCount'] == self.mn_count - 1, timeout=5)
        more_triggers = self.wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) > 1, timeout=5, do_assert=False)
        assert_equal(more_triggers, False)
        assert_equal(self.nodes[0].gobject("count")["votes"], 19)
        assert_equal(isolated.gobject("count")["votes"], 16)

        self.reconnect_isolated_node(payee_idx, 0)
        # self.connect_nodes(0, payee_idx)
        self.sync_blocks()

        # re-sync helper
        def sync_gov(node):
            self.bump_mocktime(1)
            return node.mnsync("status")["IsSynced"]

        self.log.info("make sure isolated node is fully synced at this point")
        self.wait_until(lambda: sync_gov(isolated))
        self.log.info("let all fulfilled requests expire for re-sync to work correctly")
        self.bump_mocktime(5 * 60)

        for node in self.nodes:
            # Force sync
            node.mnsync("reset")
            # fast-forward to governance sync
            node.mnsync("next")
            self.wait_until(lambda: sync_gov(node))

        self.log.info("Should see two triggers now")
        self.wait_until(lambda: len(isolated.gobject("list", "valid", "triggers")) == 2, timeout=5)
        self.wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) == 2, timeout=5)
        more_triggers = self.wait_until(lambda: len(self.nodes[0].gobject("list", "valid", "triggers")) > 2, timeout=5, do_assert=False)
        assert_equal(more_triggers, False)

        self.log.info("Move another block inside the Superblock maturity window")
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())

        self.log.info("Should see same YES and NO vote count for both triggers on all nodes now")
        for node in self.nodes:
            self.wait_until(lambda: node.gobject("list", "valid", "triggers")[winning_trigger_hash]['YesCount'] == self.mn_count - 1, timeout=5)
            self.wait_until(lambda: node.gobject("list", "valid", "triggers")[winning_trigger_hash]['NoCount'] == 1, timeout=5)
            self.wait_until(lambda: node.gobject("list", "valid", "triggers")[isolated_trigger_hash]['YesCount'] == 1, timeout=5)
            self.wait_until(lambda: node.gobject("list", "valid", "triggers")[isolated_trigger_hash]['NoCount'] == self.mn_count - 1, timeout=5)

        self.log.info("Should have 25 votes on all nodes")
        for node in self.nodes:
            assert_equal(node.gobject("count")["votes"], 25)

        self.log.info("Remember vote count")
        before = self.nodes[1].gobject("count")["votes"]

        self.log.info("Bump mocktime to let MNs vote again")
        self.bump_mocktime(GOVERNANCE_UPDATE_MIN + 1, update_schedulers=False)

        self.log.info("Move another block inside the Superblock maturity window")
        with self.nodes[1].assert_debug_log(["CGovernanceManager::VoteGovernanceTriggers"]):
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())

        self.log.info("Vote count should not change even though MNs are allowed to vote again")
        assert_equal(before, self.nodes[1].gobject("count")["votes"])
        self.log.info("Revert mocktime back to avoid issues in tests below")
        self.bump_mocktime(GOVERNANCE_UPDATE_MIN * -1, update_schedulers=False)

        block_count = self.nodes[0].getblockcount()
        n = sb_cycle - block_count % sb_cycle

        self.log.info("Move remaining n blocks until actual Superblock")
        for i in range(n):
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
            # comparing to 159 because bip9 forks are active when the tip is one block behind the activation height
            self.check_superblockbudget(block_count + i + 1 >= 159)

        self.check_superblockbudget(True)
        self.check_superblock()

        self.log.info("Move a few block past the recent superblock height and make sure we have no new votes")
        for _ in range(5):
            with self.nodes[1].assert_debug_log(expected_msgs=[""], unexpected_msgs=[f"Voting NO-FUNDING for trigger:{winning_trigger_hash} success"]):
                self.bump_mocktime(1)
                self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
            # Votes on both triggers should NOT change
            assert_equal(self.nodes[0].gobject("list", "valid", "triggers")[winning_trigger_hash]['NoCount'], 1)
            assert_equal(self.nodes[0].gobject("list", "valid", "triggers")[isolated_trigger_hash]['NoCount'], self.mn_count - 1)

        block_count = self.nodes[0].getblockcount()
        n = sb_cycle - block_count % sb_cycle

        self.log.info("Move remaining n blocks until the next Superblock")
        for _ in range(n - 1):
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
        self.log.info("Wait for new trigger and votes")
        self.wait_until(lambda: have_trigger_for_height(self.nodes, 180))
        self.log.info("Mine superblock")
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
        assert_equal(self.nodes[0].getblockcount(), 180)
        assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["active"], True)

        self.log.info("Mine and check a couple more superblocks")
        for i in range(2):
            for _ in range(sb_cycle - 1):
                self.bump_mocktime(1)
                self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
            # Wait for new trigger and votes
            sb_block_height = 180 + (i + 1) * sb_cycle
            self.wait_until(lambda: have_trigger_for_height(self.nodes, sb_block_height))
            # Mine superblock
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=self.sync_blocks())
            assert_equal(self.nodes[0].getblockcount(), sb_block_height)
            assert_equal(self.nodes[0].getblockchaininfo()["softforks"]["v20"]["active"], True)
            self.check_superblockbudget(True)
            self.check_superblock()


if __name__ == '__main__':
    DashGovernanceTest().main()
