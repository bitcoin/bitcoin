#!/usr/bin/env python3
# Copyright (c) 2018-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests around dash governance."""
import time
import json
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, satoshi_round, wait_until_helper_internal
from decimal import Decimal

class SyscoinGovernanceTest (DashTestFramework):
    def set_test_params(self):
        # using adjusted v20 deployment params to test an edge case where superblock maturity window is equal to deployment window size
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def add_options(self, parser):
        self.add_wallet_options(parser)
        
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
            "url": "https://syscoin.org"
        }
        proposal_hex = ''.join(format(x, '02x') for x in json.dumps(proposal_template).encode())
        collateral_hash = self.nodes[0].gobject_prepare(parent_hash, proposal_rev, proposal_time, proposal_hex)
        return {
            "parentHash": parent_hash,
            "collateralHash": collateral_hash,
            "createdAt": proposal_time,
            "revision": proposal_rev,
            "hex": proposal_hex,
            "data": proposal_template,
        }

    def check_superblockbudget(self):
        assert_equal(self.nodes[0].getsuperblockbudget(), self.budget)

    def check_superblock(self):
        # Make sure Superblock has only payments that fit into the budget
        # p0 must always be included because it has most votes
        # p1 and p2 have equal number of votes (but less votes than p0),
        # so exactly one of them must be included.
        coinbase_outputs = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]["vout"]
        payments_found = 0
        payment_value = 0
        found_p0 = False
        found_p1 = False
        found_p2 = False
        for txout in coinbase_outputs:
            if txout["value"] == self.p0_amount and txout["scriptPubKey"]["address"] == self.p0_payout_address:
                payments_found += 1
                payment_value += self.p0_amount
                found_p0 = True
            if txout["value"] == self.p1_amount and txout["scriptPubKey"]["address"] == self.p1_payout_address:
                payments_found += 1
                payment_value += self.p1_amount
                found_p1 = True
            if txout["value"] == self.p2_amount and txout["scriptPubKey"]["address"] == self.p2_payout_address:
                payments_found += 1
                payment_value += self.p2_amount
                found_p2 = True
        if payment_value <= self.budget*Decimal("0.95"):
            self.budget = self.budget*Decimal("0.9")
        elif payment_value >= self.budget*Decimal("1.05"):
            self.budget = self.budget*Decimal("1.1")
        assert_equal(found_p0, True)
        # Equal-vote proposals may both be paid when budget/headroom allows for this cycle.
        # Keep strict checks: p0 must be paid and p1/p2 must include at least one winner.
        p1p2_found = int(found_p1) + int(found_p2)
        assert p1p2_found in (1, 2)
        if p1p2_found == 2:
            assert payment_value <= self.budget * Decimal("1.05")
        assert_equal(payments_found, 1 + p1p2_found)

    def have_trigger_for_height(self, sb_block_height):
        count = 0
        for node in self.nodes:
            valid_triggers = node.gobject_list("valid", "triggers")
            for trigger in list(valid_triggers.values()):
                if json.loads(trigger["DataString"])["event_block_height"] != sb_block_height:
                    continue
                if trigger['AbsoluteYesCount'] > 0:
                    count = count + 1
                    break
        return count == len(self.nodes)

    def wait_for_trigger(self, sb_block_height, timeout=10):
        def check_for_trigger():
            if((self.nodes[0].getblockcount()+1) >= sb_block_height):
                return False
            self.bump_mocktime(6)
            self.sync_blocks()
            time.sleep(2)
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            return self.have_trigger_for_height(sb_block_height)
        wait_until_helper_internal(check_for_trigger, timeout=timeout)
        
    def run_test(self):
        self.budget = satoshi_round("2000000.00")
        governance_info = self.nodes[0].getgovernanceinfo()
        assert_equal(governance_info['governanceminquorum'], 1)
        assert_equal(governance_info['proposalfee'], 150.0)
        assert_equal(governance_info['superblockcycle'], 25)
        assert_equal(governance_info['superblockmaturitywindow'], 5)
        assert_equal(governance_info['lastsuperblock'], 125)
        assert_equal(governance_info['nextsuperblock'], governance_info['lastsuperblock'] + governance_info['superblockcycle'])
        assert_equal(governance_info['governancebudget'], self.budget)

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
        sb_cycle = governance_info['superblockcycle']
        sb_maturity_window = governance_info['superblockmaturitywindow']
        sb_immaturity_window = sb_cycle - sb_maturity_window
        self.expected_v20_budget = satoshi_round("2200000.00")


        self.nodes[0].spork("SPORK_9_SUPERBLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        assert_equal(len(self.nodes[0].gobject_list_prepared()), 0)

        self.generate(self.nodes[0], 3)
        self.bump_mocktime(3)
        self.sync_blocks()
        assert_equal(self.nodes[0].getblockcount(), 130)
        self.check_superblockbudget()


        proposal_time = self.mocktime
        self.p0_payout_address = self.nodes[0].getnewaddress()
        self.p1_payout_address = self.nodes[0].getnewaddress()
        self.p2_payout_address = self.nodes[0].getnewaddress()
        self.p0_amount = satoshi_round("1.1")
        self.p1_amount = satoshi_round("3.3")
        self.p2_amount = self.expected_v20_budget - self.p1_amount

        p0_collateral_prepare = self.prepare_object(1, "%064x" % 0, proposal_time, 1, "Proposal_0", self.p0_amount, self.p0_payout_address)
        p1_collateral_prepare = self.prepare_object(1, "%064x" % 0, proposal_time, 1, "Proposal_1", self.p1_amount, self.p1_payout_address)
        p2_collateral_prepare = self.prepare_object(1, "%064x" % 0, proposal_time, 1, "Proposal_2", self.p2_amount, self.p2_payout_address)

        self.generate(self.nodes[0], 6)
        self.bump_mocktime(6)
        self.sync_blocks()

        assert_equal(len(self.nodes[0].gobject_list_prepared()), 3)
        assert_equal(len(self.nodes[0].gobject_list()), 0)

        self.p0_hash = self.nodes[0].gobject_submit("0", 1, proposal_time, p0_collateral_prepare["hex"], p0_collateral_prepare["collateralHash"])
        self.p1_hash = self.nodes[0].gobject_submit("0", 1, proposal_time, p1_collateral_prepare["hex"], p1_collateral_prepare["collateralHash"])
        self.p2_hash = self.nodes[0].gobject_submit("0", 1, proposal_time, p2_collateral_prepare["hex"], p2_collateral_prepare["collateralHash"])
        
        def sync_gobject_list(node):
            self.bump_mocktime(1)
            return len(node.gobject_list()) == 3

        for i in range(len(self.nodes)):
            self.wait_until(lambda: sync_gobject_list(self.nodes[i]), timeout=5)

        assert_equal(self.nodes[0].gobject_get(self.p0_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject_get(self.p0_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject_get(self.p1_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject_get(self.p1_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject_get(self.p2_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject_get(self.p2_hash)["FundingResult"]["NoCount"], 0)

        self.nodes[0].gobject_vote_alias(self.p0_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject_vote_many(self.p0_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject_get(self.p0_hash)["FundingResult"]["YesCount"], self.mn_count - 1)
        assert_equal(self.nodes[0].gobject_get(self.p0_hash)["FundingResult"]["NoCount"], 1)

        self.nodes[0].gobject_vote_alias(self.p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject_vote_alias(self.p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject_vote_many(self.p1_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject_get(self.p1_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject_get(self.p1_hash)["FundingResult"]["NoCount"], 2)

        self.nodes[0].gobject_vote_alias(self.p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject_vote_alias(self.p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject_vote_many(self.p2_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject_get(self.p2_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject_get(self.p2_hash)["FundingResult"]["NoCount"], 2)

        assert_equal(len(self.nodes[0].gobject_list("valid", "triggers")), 0)

        block_count = self.nodes[0].getblockcount()

        # Move until 1 block before the Superblock maturity window starts
        n = sb_immaturity_window - block_count % sb_cycle
        assert block_count + n < 150
        for _ in range(n - 1):
            self.generate(self.nodes[0], 1)
            self.bump_mocktime(1)
            self.sync_blocks()

        assert_equal(len(self.nodes[0].gobject_list("valid", "triggers")), 0)

        # Detect payee node
        mn_list = self.nodes[0].protx_list("registered", True)
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
        isolated = self.nodes[payee_idx]
        non_isolated_nodes = [node for idx, node in enumerate(self.nodes) if idx != payee_idx]

        self.isolate_node(isolated)
        for idx, node in enumerate(non_isolated_nodes):
            if node.index != 0:
                self.connect_nodes(0, node.index, wait_for_connect=False)
                self.connect_nodes(node.index, 0, wait_for_connect=False)
        # Move 1 block inside the Superblock maturity window on the isolated node
        self.generate(isolated, 1, sync_fun=self.no_op)
        def sync_gobject_list_trigger(node, count):
            self.bump_mocktime(1)
            return len(node.gobject_list("valid", "triggers")) == count
            
        # The isolated "winner" should submit new trigger and vote for it
        self.wait_until(lambda: sync_gobject_list_trigger(isolated, 1), timeout=5)
        isolated_trigger_hash = list(isolated.gobject_list("valid", "triggers").keys())[0]
        self.wait_until(lambda: list(isolated.gobject_list("valid", "triggers").values())[0]['YesCount'] == 1, timeout=5)
        more_votes = wait_until_helper_internal(lambda: list(isolated.gobject_list("valid", "triggers").values())[0]['YesCount'] > 1, timeout=5, do_assert=False)
        assert_equal(more_votes, False)

        # Move 1 block enabling the Superblock maturity window on non-isolated nodes
        # make sure no triggers exist before block
        for idx, node in enumerate(non_isolated_nodes):
            self.wait_until(lambda: sync_gobject_list_trigger(node, 0), timeout=5)
            
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.sync_all(nodes=non_isolated_nodes)
        self.bump_mocktime(1)
        assert_equal(self.nodes[0].getblockcount(), 145)

        # The "winner" should submit new trigger and vote for it, but payee which is same as last is isolated so non-isolated nodes will not be in payee list
        has_trigger = wait_until_helper_internal(lambda: len(self.nodes[0].gobject_list("valid", "triggers")) >= 1, timeout=5, do_assert=False)
        assert_equal(has_trigger, False)
        
        # Move 1 block inside the Superblock maturity window on non-isolated nodes
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.sync_all(nodes=non_isolated_nodes)

        # There is now new "winner" who should submit new trigger and vote for it
        for idx, node in enumerate(non_isolated_nodes):
            self.wait_until(lambda: sync_gobject_list_trigger(node, 1), timeout=5)
        winning_trigger_hash = list(self.nodes[0].gobject_list("valid", "triggers").keys())[0]
        self.wait_until(lambda: list(self.nodes[0].gobject_list("valid", "triggers").values())[0]['YesCount'] == 1, timeout=5)
        more_votes = wait_until_helper_internal(lambda: list(self.nodes[0].gobject_list("valid", "triggers").values())[0]['YesCount'] > 1, timeout=5, do_assert=False)
        assert_equal(more_votes, False)

        # Make sure amounts aren't trimmed
        payment_amounts_expected = [str(satoshi_round(str(self.p0_amount))), str(satoshi_round(str(self.p1_amount))), str(satoshi_round(str(self.p2_amount)))]
        data_string = list(self.nodes[0].gobject_list("valid", "triggers").values())[0]["DataString"]
        payment_amounts_trigger = json.loads(data_string)["payment_amounts"].split("|")
        for amount_str in payment_amounts_trigger:
            assert(amount_str in payment_amounts_expected)

        # Move another block inside the Superblock maturity window on non-isolated nodes
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.sync_all(nodes=non_isolated_nodes)
        def sync_gobject_list2(node):
            self.bump_mocktime(1)
            count = list(node.gobject_list("valid", "triggers").values())[0]['YesCount']
            return count == self.mn_count - 1
    
        # Every non-isolated MN should vote for the same trigger now, no new triggers should be created
        self.wait_until(lambda: sync_gobject_list2(self.nodes[0]), timeout=5)
        more_triggers = wait_until_helper_internal(lambda: len(self.nodes[0].gobject_list("valid", "triggers")) > 1, timeout=5, do_assert=False)
        assert_equal(more_triggers, False)

        self.reconnect_isolated_node(self.nodes[payee_idx], 0)
        for idx, node in enumerate(self.nodes):
            if node.index != 0:
                self.connect_nodes(0, node.index, wait_for_connect=False)
                self.connect_nodes(node.index, 0, wait_for_connect=False)
        self.sync_blocks()

        # re-sync helper
        def sync_gov(node):
            self.bump_mocktime(1)
            return node.mnsync("status")["IsSynced"]

        # make sure isolated node is fully synced at this point
        self.wait_until(lambda: sync_gov(isolated), timeout=180)
        # let all fulfilled requests expire for re-sync to work correctly
        self.bump_mocktime(5 * 60)
        for node in self.nodes:
            # Force sync
            node.mnsync("reset")
            # fast-forward to governance sync
            node.mnsync("next")
            self.wait_until(lambda: sync_gov(node), timeout=180)

        # Should see two triggers now
        self.wait_until(lambda: sync_gobject_list_trigger(isolated, 2), timeout=5)
        self.wait_until(lambda: sync_gobject_list_trigger(self.nodes[0], 2), timeout=5)
        more_triggers = wait_until_helper_internal(lambda: len(self.nodes[0].gobject_list("valid", "triggers")) > 2, timeout=5, do_assert=False)
        assert_equal(more_triggers, False)

        # Move another block inside the Superblock maturity window
        self.generate(self.nodes[0], 1)
        self.bump_mocktime(1)
        self.sync_blocks()

        def sync_gobject_list3(node, trigger_hash, count):
            self.bump_mocktime(1)
            return node.gobject_list("valid", "triggers")[trigger_hash]['NoCount'] == count
        
        # Should see NO votes on both triggers now
        self.wait_until(lambda: sync_gobject_list3(self.nodes[0], winning_trigger_hash, 1), timeout=5)
        self.wait_until(lambda: sync_gobject_list3(self.nodes[0], isolated_trigger_hash, self.mn_count - 1), timeout=5)

        block_count = self.nodes[0].getblockcount()
        n = sb_cycle - block_count % sb_cycle

        # Move remaining n blocks until actual Superblock
        for i in range(n):
            self.generate(self.nodes[0], 1)
            self.bump_mocktime(1)
            self.sync_blocks()

        self.check_superblock()
        self.check_superblockbudget()

        # Move a few block past the recent superblock height and make sure we have no new votes
        for _ in range(5):
            with self.nodes[1].assert_debug_log(expected_msgs=[], unexpected_msgs=[f"Voting NO-FUNDING for trigger:{winning_trigger_hash} success"]):
                self.generate(self.nodes[0], 1)
                self.bump_mocktime(1)
                self.sync_blocks()
            # Votes on both triggers should NOT change
            assert_equal(self.nodes[0].gobject_list("valid", "triggers")[winning_trigger_hash]['NoCount'], 1)
            assert_equal(self.nodes[0].gobject_list("valid", "triggers")[isolated_trigger_hash]['NoCount'], self.mn_count - 1)

        block_count = self.nodes[0].getblockcount()
        gov_info = self.nodes[0].getgovernanceinfo()
        sb_cycle = gov_info['superblockcycle']
        sb_height = gov_info['nextsuperblock']

        # Move until 1 block before the Superblock maturity window starts
        n = sb_immaturity_window - block_count % sb_cycle
        self.generate(self.nodes[0], n - 1)
        self.log.info(f"Waiting for trigger")
        self.wait_for_trigger(sb_height)
        
        # Mine superblock
        block_count = self.nodes[0].getblockcount()
        n = sb_height - block_count
        if n > 0:
            self.generate(self.nodes[0], n)
        assert_equal(self.nodes[0].getblockcount(), 175)
        self.check_superblock()
        self.check_superblockbudget()
        # Mine and check a couple more superblocks
        for i in range(2):
            block_count = self.nodes[0].getblockcount()
            gov_info = self.nodes[0].getgovernanceinfo()
            sb_cycle = gov_info['superblockcycle']
            sb_height = gov_info['nextsuperblock']

            # Move until 1 block before the Superblock maturity window starts
            n = sb_immaturity_window - block_count % sb_cycle
            self.generate(self.nodes[0], n - 1)
            self.log.info(f"Waiting for trigger")
            self.wait_for_trigger(sb_height)
            block_count = self.nodes[0].getblockcount()
            n = sb_height - block_count
            if n > 0:
                self.generate(self.nodes[0], n)
            self.log.info(f"Mined superblock at height {sb_height}")
            assert_equal(self.nodes[0].getblockcount(), sb_height)
            self.check_superblock()
            self.check_superblockbudget()


if __name__ == '__main__':
    SyscoinGovernanceTest().main()