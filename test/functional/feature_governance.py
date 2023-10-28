#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests around dash governance."""

import json
import time

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
            "end_epoch": proposal_time + 24 * 60 * 60,
            "payment_amount": amount,
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

        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 4070908800)
        self.nodes[0].sporkupdate("SPORK_9_SUPERBLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 0)

        proposal_time = self.mocktime
        sb_block_height = self.nodes[0].getblockcount() + 10 - self.nodes[0].getblockcount() % 10
        p0_payout_address = self.nodes[0].getnewaddress()
        p1_payout_address = self.nodes[0].getnewaddress()
        p2_payout_address = self.nodes[0].getnewaddress()
        p0_amount = float(1.1)
        p1_amount = float(3.3)
        p2_amount = float(self.nodes[0].getsuperblockbudget(sb_block_height)) - p1_amount

        p0_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_0", p0_amount, p0_payout_address)
        p1_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_1", p1_amount, p1_payout_address)
        p2_collateral_prepare = self.prepare_object(1, uint256_to_string(0), proposal_time, 1, "Proposal_2", p2_amount, p2_payout_address)

        self.nodes[0].generate(6)
        self.sync_blocks()

        assert_equal(len(self.nodes[0].gobject("list-prepared")), 3)
        assert_equal(len(self.nodes[0].gobject("list")), 0)

        p0_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p0_collateral_prepare["hex"], p0_collateral_prepare["collateralHash"])
        p1_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p1_collateral_prepare["hex"], p1_collateral_prepare["collateralHash"])
        p2_hash = self.nodes[0].gobject("submit", "0", 1, proposal_time, p2_collateral_prepare["hex"], p2_collateral_prepare["collateralHash"])

        assert_equal(len(self.nodes[0].gobject("list")), 3)

        assert_equal(self.nodes[0].gobject("get", p0_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", p0_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject("get", p1_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", p1_hash)["FundingResult"]["NoCount"], 0)

        assert_equal(self.nodes[0].gobject("get", p2_hash)["FundingResult"]["YesCount"], 0)
        assert_equal(self.nodes[0].gobject("get", p2_hash)["FundingResult"]["NoCount"], 0)

        self.nodes[0].gobject("vote-alias", p0_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-many", p0_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", p0_hash)["FundingResult"]["YesCount"], self.mn_count - 1)
        assert_equal(self.nodes[0].gobject("get", p0_hash)["FundingResult"]["NoCount"], 1)

        self.nodes[0].gobject("vote-alias", p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-alias", p1_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject("vote-many", p1_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", p1_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject("get", p1_hash)["FundingResult"]["NoCount"], 2)

        self.nodes[0].gobject("vote-alias", p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[0].proTxHash)
        self.nodes[0].gobject("vote-alias", p2_hash, map_vote_signals[1], map_vote_outcomes[2], self.mninfo[1].proTxHash)
        self.nodes[0].gobject("vote-many", p2_hash, map_vote_signals[1], map_vote_outcomes[1])
        assert_equal(self.nodes[0].gobject("get", p2_hash)["FundingResult"]["YesCount"], self.mn_count - 2)
        assert_equal(self.nodes[0].gobject("get", p2_hash)["FundingResult"]["NoCount"], 2)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)

        block_count = self.nodes[0].getblockcount()
        sb_cycle = 10
        sb_maturity_window = 2
        sb_maturity_cycle = sb_cycle - sb_maturity_window

        # Move until 1 block before the Superblock maturity window starts
        n = sb_maturity_cycle - block_count % sb_cycle
        self.nodes[0].generate(n - 1)
        self.sync_blocks()
        time.sleep(1)

        assert_equal(len(self.nodes[0].gobject("list", "valid", "triggers")), 0)

        # Move 1 block enabling the Superblock maturity window
        self.nodes[0].generate(1)
        self.sync_blocks()
        time.sleep(1)

        # The "winner" should submit new trigger and vote for it, no one else should vote yet
        valid_triggers = self.nodes[0].gobject("list", "valid", "triggers")
        assert_equal(len(valid_triggers), 1)
        trigger_data = list(valid_triggers.values())[0]
        assert_equal(trigger_data['YesCount'], 1)

        # Make sure amounts aren't trimmed
        payment_amounts_expected = [str(satoshi_round(str(p0_amount))), str(satoshi_round(str(p1_amount))), str(satoshi_round(str(p2_amount)))]
        data_string = list(self.nodes[0].gobject("list", "valid", "triggers").values())[0]["DataString"]
        payment_amounts_trigger = json.loads(data_string)["payment_amounts"].split("|")
        for amount_str in payment_amounts_trigger:
            assert(amount_str in payment_amounts_expected)

        # Move 1 block inside the Superblock maturity window
        self.nodes[0].generate(1)
        self.sync_blocks()
        time.sleep(1)

        # Every MN should vote for the same trigger now, no new triggers should be created
        triggers_rpc = self.nodes[0].gobject("list", "valid", "triggers")
        assert_equal(len(triggers_rpc), 1)
        trigger_data = list(triggers_rpc.values())[0]
        assert_equal(trigger_data['YesCount'], self.mn_count)

        block_count = self.nodes[0].getblockcount()
        n = sb_cycle - block_count % sb_cycle

        # Move remaining n blocks until actual Superblock
        for i in range(n):
            time.sleep(1)
            self.nodes[0].generate(1)
            self.sync_blocks()

        # Make sure Superblock has only payments that fit into the budget
        # p0 must always be included because it has most votes
        # p1 and p2 have equal number of votes (but less votes than p0)
        # so only one of them can be included (depends on proposal hashes).
        coinbase_outputs = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]["vout"]
        payments_found = 0
        for txout in coinbase_outputs:
            if txout["value"] == satoshi_round(str(p0_amount)) and txout["scriptPubKey"]["addresses"][0] == p0_payout_address:
                payments_found += 1
            if txout["value"] == satoshi_round(str(p1_amount)) and txout["scriptPubKey"]["addresses"][0] == p1_payout_address:
                if p1_hash > p2_hash:
                    payments_found += 1
                else:
                    assert False
            if txout["value"] == satoshi_round(str(p2_amount)) and txout["scriptPubKey"]["addresses"][0] == p2_payout_address:
                if p2_hash > p1_hash:
                    payments_found += 1
                else:
                    assert False

        assert_equal(payments_found, 2)


if __name__ == '__main__':
    DashGovernanceTest().main()
