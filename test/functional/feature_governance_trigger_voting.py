#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test how masternodes vote on superblock triggers when their approved proposal set changes.

A masternode creates or votes on triggers only from its block tip callback. To control which
masternodes have acted on a block before a trigger exists, the masternode that creates a trigger
is stopped while the block is mined and started again afterwards, so it creates the trigger while
catching up. A masternode that is stopped while proposal votes are cast keeps its earlier view of
the approved proposals. Each scenario uses its own short-lived proposals, so such a view only
matters within that scenario.
"""

import json

from test_framework.test_framework import BitcoinTestFramework, DashTestFramework
from test_framework.util import assert_equal

BUDGET_ARGS = ["-budgetparams=10:10:10"]
SB_CYCLE = 20
SB_MATURITY_WINDOW = 10
NO_VOTE_DEADLINE_BLOCKS = 3  # src/governance/signing.cpp
GOVERNANCE_FEE_CONFIRMATIONS = 6
PROPOSAL_LIFETIME = 60 * 60
# Long enough for the proposals of the previous scenario to leave the payment window, and for every
# masternode to get clear of the trigger rate limit
SCENARIO_GAP = 4 * 60 * 60


class GovernanceTriggerVotingTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.set_dash_test_params(6, 5, [BUDGET_ARGS] * 6)

    def online_nodes(self):
        return [node for node in self.nodes if node.running]

    def online_mns(self):
        return [mn for mn in self.mninfo if self.nodes[mn.nodeIdx].running]

    def flush_relay(self):
        for node in self.online_nodes():
            node.mockscheduler(1)

    def wait_relayed(self, predicate):
        def check():
            # Runs the once-a-second governance relay flush on every node
            self.flush_relay()
            return predicate()
        self.wait_until(check, timeout=10)

    def mine(self):
        nodes = self.online_nodes()
        self.bump_mocktime(1, nodes=nodes)
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(nodes))
        # Masternodes act on a block only from the validation callback
        for node in nodes:
            node.syncwithvalidationinterfacequeue()

    def mine_to(self, height):
        while self.nodes[0].getblockcount() < height:
            self.mine()
        assert_equal(self.nodes[0].getblockcount(), height)

    def connect_to_controller(self, idx):
        running = [self.nodes[mn.nodeIdx] for mn in self.online_mns() if mn.nodeIdx != idx]
        for node in running:
            node.setmnthreadactive(False)
        BitcoinTestFramework.connect_nodes(self, idx, 0)
        for node in running:
            node.setmnthreadactive(True)

    def stop_mn(self, mn):
        self.stop_node(mn.nodeIdx)

    def start_mn(self, mn):
        node = self.nodes[mn.nodeIdx]
        node.mocktime = self.mocktime
        self.start_masternode(mn)
        self.connect_to_controller(mn.nodeIdx)
        self.sync_blocks(self.online_nodes())
        node.syncwithvalidationinterfacequeue()

    def restart_mn(self, mn):
        self.stop_mn(mn)
        self.start_mn(mn)

    def payee(self, block_height):
        winners = self.nodes[0].masternode("winners")
        # Payees are projected for one rotation only, and each masternode is paid once per rotation
        while str(block_height) not in winners:
            block_height -= len(self.mninfo)
        payee_address = winners[str(block_height)]
        matches = [mn for mn in self.mninfo if self.payout_address[mn.proTxHash] in payee_address]
        assert_equal(len(matches), 1)
        return matches[0]

    def own_vote(self, mn, trigger_hash):
        """The masternode's own funding vote on the object, as known to its own node."""
        collateral_hash, collateral_index = self.collateral[mn.proTxHash]
        votes = self.nodes[mn.nodeIdx].gobject("getcurrentvotes", trigger_hash, collateral_hash, str(collateral_index))
        outcomes = [v.split(":")[2] for v in votes.values() if v.split(":")[3] == "funding"]
        assert len(outcomes) <= 1
        return outcomes[0] if outcomes else None

    def triggers_for(self, node, sb_height):
        return {h: t for h, t in node.gobject("list", "valid", "triggers").items()
                if json.loads(t["DataString"])["event_block_height"] == sb_height}

    def trigger_addresses(self, trigger_hash):
        data = json.loads(self.nodes[0].gobject("get", trigger_hash)["DataString"])
        return set(data["payment_addresses"].split("|"))

    def wait_trigger_everywhere(self, trigger_hash):
        self.wait_relayed(lambda: all(trigger_hash in node.gobject("list", "valid", "triggers")
                                      for node in self.online_nodes()))

    def paid_addresses(self, height):
        assert_equal(self.nodes[0].getblockcount(), height)
        coinbase = self.nodes[0].getblock(self.nodes[0].getblockhash(height), 2)["tx"][0]
        return {txout["scriptPubKey"].get("address") for txout in coinbase["vout"]}

    def create_proposals(self, count):
        node = self.nodes[0]
        created_at = self.mocktime
        prepared = []
        for _ in range(count):
            address = node.getnewaddress()
            proposal = {
                "type": 1,
                "name": f"Proposal_{len(self.proposal_address)}",
                "start_epoch": created_at,
                "end_epoch": created_at + PROPOSAL_LIFETIME,
                "payment_amount": 1,
                "payment_address": address,
                "url": "https://dash.org",
            }
            proposal_hex = json.dumps(proposal).encode().hex()
            collateral = node.gobject("prepare", "0", 1, created_at, proposal_hex)
            prepared.append((proposal_hex, collateral, address))
        self.sync_mempools(self.online_nodes())
        # Collateral transactions without an InstantSend lock are only mined once they are this old
        self.bump_mocktime(10 * 60 + 1, nodes=self.online_nodes())
        for _ in range(GOVERNANCE_FEE_CONFIRMATIONS):
            self.mine()
        proposals = []
        for proposal_hex, collateral, address in prepared:
            proposal_hash = node.gobject("submit", "0", 1, created_at, proposal_hex, collateral)
            self.proposal_address[proposal_hash] = address
            proposals.append(proposal_hash)
        self.wait_relayed(lambda: all(set(proposals) <= set(n.gobject("list", "valid", "proposals"))
                                      for n in self.online_nodes()))
        return proposals

    def set_approval(self, changes):
        """Approve (True) or drop (False) proposals, each change cast with a voting key that has not
        voted on that proposal yet, and wait until every running node sees the result."""
        for proposal_hash, approved in changes.items():
            voter = self.mninfo[self.proposal_voters.get(proposal_hash, 0)]
            self.proposal_voters[proposal_hash] = self.proposal_voters.get(proposal_hash, 0) + 1
            result = self.nodes[0].gobject("vote-alias", proposal_hash, "funding", "yes" if approved else "no",
                                           voter.proTxHash)
            assert_equal(result["overall"], "Voted successfully 1 time(s) and failed 0 time(s).")

        def seen_everywhere():
            return all((n.gobject("get", h)["FundingResult"]["AbsoluteYesCount"] >= 1) == approved
                       for h, approved in changes.items() for n in self.online_nodes())
        self.wait_relayed(seen_everywhere)

    def start_scenario(self, proposal_count):
        assert_equal(len(self.online_nodes()), self.num_nodes)
        self.bump_mocktime(SCENARIO_GAP, update_schedulers=False)
        proposals = self.create_proposals(proposal_count)
        tip = self.nodes[0].getblockcount()
        sb_height = tip - tip % SB_CYCLE + SB_CYCLE
        if sb_height - SB_MATURITY_WINDOW - 1 <= tip:
            sb_height += SB_CYCLE
        return sb_height, proposals

    def create_first_trigger(self, sb_height):
        """Mine the first block of the maturity window while its trigger creator is stopped, then start
        the creator. Every other running masternode has acted on that block before the trigger exists."""
        window_start = sb_height - SB_MATURITY_WINDOW
        assert_equal(self.nodes[0].getblockcount(), window_start - 1)
        creator = self.payee(window_start + 1)
        self.stop_mn(creator)
        self.mine()
        assert_equal(self.triggers_for(self.nodes[0], sb_height), {})
        self.start_mn(creator)
        triggers = self.triggers_for(self.nodes[creator.nodeIdx], sb_height)
        assert_equal(len(triggers), 1)
        trigger = next(iter(triggers))
        self.wait_trigger_everywhere(trigger)
        assert_equal(self.own_vote(creator, trigger), "yes")
        for mn in self.online_mns():
            if mn is not creator:
                assert_equal(self.own_vote(mn, trigger), None)
        return creator, trigger

    def run_test(self):
        node = self.nodes[0]
        governance_info = node.getgovernanceinfo()
        assert_equal(governance_info["superblockcycle"], SB_CYCLE)
        assert_equal(governance_info["superblockmaturitywindow"], SB_MATURITY_WINDOW)
        assert_equal(governance_info["governanceminquorum"], 1)

        protx_info = {mn.proTxHash: node.protx("info", mn.proTxHash) for mn in self.mninfo}
        self.payout_address = {h: info["state"]["payoutAddress"] for h, info in protx_info.items()}
        self.collateral = {h: (info["collateralHash"], info["collateralIndex"]) for h, info in protx_info.items()}
        self.proposal_address = {}
        self.proposal_voters = {}

        self.test_single_change_is_snapshot()
        self.test_creator_votes_after_unfunded_cycle(restart=False)
        self.test_creator_votes_after_unfunded_cycle(restart=True)
        self.test_own_no_vote_is_kept()
        self.test_yes_vote_survives_tip_below_window()
        # Leaves a masternode that does not know the trigger the superblock pays, so it runs last
        self.test_no_votes_wait_for_deadline()

    def test_single_change_is_snapshot(self):
        self.log.info("A single change of the approved set: the trigger for the later set is paid")
        sb_height, (p_kept, p_dropped) = self.start_scenario(2)
        self.set_approval({p_kept: True, p_dropped: True})
        self.mine_to(sb_height - SB_MATURITY_WINDOW - 1)
        creator, first_trigger = self.create_first_trigger(sb_height)
        assert_equal(self.trigger_addresses(first_trigger),
                     {self.proposal_address[p_kept], self.proposal_address[p_dropped]})

        self.set_approval({p_dropped: False})
        second_creator = self.payee(self.nodes[0].getblockcount() + 2)
        self.mine()
        triggers = self.triggers_for(self.nodes[second_creator.nodeIdx], sb_height)
        assert_equal(len(triggers), 2)
        second_trigger = next(h for h in triggers if h != first_trigger)
        self.wait_trigger_everywhere(second_trigger)
        assert_equal(self.trigger_addresses(second_trigger), {self.proposal_address[p_kept]})
        self.mine()

        for node in self.nodes:
            assert_equal(set(self.triggers_for(node, sb_height)), {first_trigger, second_trigger})
        for mn in self.mninfo:
            if mn is creator:
                assert_equal(self.own_vote(mn, first_trigger), "yes")
                assert_equal(self.own_vote(mn, second_trigger), "no")
            else:
                assert_equal(self.own_vote(mn, first_trigger), "no")
                assert_equal(self.own_vote(mn, second_trigger), "yes")
        self.mine_to(sb_height)
        paid = self.paid_addresses(sb_height)
        assert self.proposal_address[p_kept] in paid
        assert self.proposal_address[p_dropped] not in paid

    def test_creator_votes_after_unfunded_cycle(self, restart):
        if restart:
            self.log.info("A trigger creator that re-learns its YES after a restart votes YES in the next cycle "
                          "after its trigger is not funded")
        else:
            self.log.info("A trigger creator votes YES in the next cycle after its trigger is not funded")
        sb_height, (proposal,) = self.start_scenario(1)
        address = self.proposal_address[proposal]
        self.set_approval({proposal: True})
        self.mine_to(sb_height - SB_MATURITY_WINDOW - 1)
        creator, trigger = self.create_first_trigger(sb_height)
        creator_node = self.nodes[creator.nodeIdx]
        if restart:
            # The creator keeps its approved set: it misses the vote that empties it
            self.stop_mn(creator)

        self.log.info("With no approved proposal, masternodes vote NO on the trigger at the next block")
        self.set_approval({proposal: False})
        self.mine()
        for mn in self.online_mns():
            if mn is not creator:
                assert_equal(self.own_vote(mn, trigger), "no")
        if restart:
            with creator_node.assert_debug_log([f"Not voting YES-FUNDING for trigger:{trigger}, we voted yes for it already"]):
                self.start_mn(creator)
        assert_equal(self.own_vote(creator, trigger), "yes")
        self.mine_to(sb_height)
        assert address not in self.paid_addresses(sb_height)

        self.log.info("The proposal is approved again: every masternode votes YES on the next trigger and it is paid")
        next_sb_height = sb_height + SB_CYCLE
        self.set_approval({proposal: True})
        self.mine_to(next_sb_height - SB_MATURITY_WINDOW - 1)

        def one_trigger_everywhere():
            self.flush_relay()
            return all(len(self.triggers_for(n, next_sb_height)) == 1 for n in self.online_nodes())
        while True:
            assert self.nodes[0].getblockcount() < next_sb_height - NO_VOTE_DEADLINE_BLOCKS - 2
            self.mine()
            if self.wait_until(one_trigger_everywhere, timeout=5, do_assert=False):
                break
        next_trigger = next(iter(self.triggers_for(self.nodes[0], next_sb_height)))
        # Every masternode knows the trigger and acts on it at this block
        self.mine()
        assert_equal(self.own_vote(creator, next_trigger), "yes")
        for mn in self.mninfo:
            assert_equal(self.own_vote(mn, next_trigger), "yes")
        self.mine_to(next_sb_height)
        assert address in self.paid_addresses(next_sb_height)

    def test_own_no_vote_is_kept(self):
        self.log.info("After the approved set is emptied and restored, a NO on the matching trigger is kept "
                      "and another trigger still gets NO")
        sb_height, (p_main, p_extra) = self.start_scenario(2)
        self.set_approval({p_main: True, p_extra: True})
        self.mine_to(sb_height - SB_MATURITY_WINDOW - 1)
        window_start = sb_height - SB_MATURITY_WINDOW
        # The masternode paid at window_start + 3 keeps the approved set {p_main, p_extra} and creates its
        # trigger once the others are back to {p_main}
        late = self.payee(window_start + 3)
        assert late is not self.payee(window_start + 1)
        self.stop_mn(late)
        self.set_approval({p_extra: False})
        creator, trigger = self.create_first_trigger(sb_height)
        assert_equal(self.trigger_addresses(trigger), {self.proposal_address[p_main]})

        self.set_approval({p_main: False})
        self.mine()
        holders = [mn for mn in self.online_mns() if mn is not creator]
        for mn in holders:
            assert_equal(self.own_vote(mn, trigger), "no")

        self.set_approval({p_main: True})
        with self.nodes[holders[0].nodeIdx].assert_debug_log([f"Not voting YES-FUNDING for trigger:{trigger}, we voted no for it already"]):
            self.mine()
        assert_equal(self.nodes[0].getblockcount(), window_start + 2)
        self.start_mn(late)
        late_triggers = self.triggers_for(self.nodes[late.nodeIdx], sb_height)
        assert_equal(len(late_triggers), 1)
        other_trigger = next(iter(late_triggers))
        self.wait_trigger_everywhere(other_trigger)
        assert_equal(self.trigger_addresses(other_trigger),
                     {self.proposal_address[p_main], self.proposal_address[p_extra]})
        self.mine()

        for mn in holders:
            assert_equal(self.own_vote(mn, trigger), "no")
            assert_equal(self.own_vote(mn, other_trigger), "no")
        assert_equal(self.own_vote(creator, trigger), "yes")
        assert_equal(self.own_vote(creator, other_trigger), "no")
        assert_equal(self.own_vote(late, other_trigger), "yes")
        self.mine_to(sb_height)
        paid = self.paid_addresses(sb_height)
        assert self.proposal_address[p_main] not in paid
        assert self.proposal_address[p_extra] not in paid

    def test_yes_vote_survives_tip_below_window(self):
        self.log.info("A YES is kept across a tip below the maturity window: no second YES after re-entering it")
        sb_height, (p_kept, p_dropped) = self.start_scenario(2)
        self.set_approval({p_kept: True, p_dropped: True})
        window_start = sb_height - SB_MATURITY_WINDOW
        self.mine_to(window_start - 1)
        creator, trigger = self.create_first_trigger(sb_height)

        fork_point = self.nodes[0].getblockhash(window_start - 1)
        for node in self.nodes:
            node.invalidateblock(fork_point)
            assert_equal(node.getblockcount(), window_start - 2)
        assert window_start - 2 > sb_height - SB_CYCLE
        self.set_approval({p_dropped: False})
        # A tip below the window
        self.mine()
        assert_equal(self.payee(window_start + 1), creator)
        # The creator is the payee again but has voted YES already, so it creates no trigger
        self.mine()
        assert_equal(set(self.triggers_for(self.nodes[creator.nodeIdx], sb_height)), {trigger})
        new_creator = self.payee(self.nodes[0].getblockcount() + 2)
        self.mine()
        new_triggers = set(self.triggers_for(self.nodes[new_creator.nodeIdx], sb_height)) - {trigger}
        assert_equal(len(new_triggers), 1)
        new_trigger = new_triggers.pop()
        self.wait_trigger_everywhere(new_trigger)
        assert_equal(self.trigger_addresses(new_trigger), {self.proposal_address[p_kept]})
        self.mine()

        for node in self.nodes:
            assert_equal(set(self.triggers_for(node, sb_height)), {trigger, new_trigger})
        assert_equal(self.own_vote(creator, trigger), "yes")
        assert_equal(self.own_vote(creator, new_trigger), "no")
        for mn in self.mninfo:
            if mn is not creator:
                assert_equal(self.own_vote(mn, new_trigger), "yes")
        self.mine_to(sb_height)
        paid = self.paid_addresses(sb_height)
        assert self.proposal_address[p_kept] in paid
        assert self.proposal_address[p_dropped] not in paid

    def test_no_votes_wait_for_deadline(self):
        self.log.info("Without a trigger for its own candidate, a masternode votes NO on other triggers only "
                      f"in the last {NO_VOTE_DEADLINE_BLOCKS} blocks before the superblock")
        sb_height, (p_a, p_b) = self.start_scenario(2)
        self.set_approval({p_a: True, p_b: True})
        self.mine_to(sb_height - SB_MATURITY_WINDOW - 1)
        # The masternode paid at sb_height - 5 keeps the approved set {p_a, p_b}: it is stopped until the
        # block before the one it is paid in, then creates its trigger while catching up
        late = self.payee(sb_height - 5)
        # The masternode paid at sb_height - 6 is restarted after the approved set changes late
        waiting = self.payee(sb_height - 6)
        assert late is not self.payee(sb_height - SB_MATURITY_WINDOW + 1)
        assert waiting is not self.payee(sb_height - SB_MATURITY_WINDOW + 1)
        self.stop_mn(late)
        self.set_approval({p_b: False})
        creator, trigger = self.create_first_trigger(sb_height)
        self.mine()
        for mn in self.online_mns():
            assert_equal(self.own_vote(mn, trigger), "yes")

        self.log.info("The approved set changes after every running masternode voted YES")
        self.set_approval({p_a: False, p_b: True})
        self.mine_to(sb_height - 7)
        # It was the payee at this block but has voted YES already, so it created no trigger
        assert_equal(set(self.triggers_for(self.nodes[waiting.nodeIdx], sb_height)), {trigger})
        self.restart_mn(waiting)
        self.mine()
        # Its candidate has no trigger, and it is not the payee
        assert_equal(set(self.triggers_for(self.nodes[waiting.nodeIdx], sb_height)), {trigger})
        self.start_mn(late)
        late_triggers = self.triggers_for(self.nodes[late.nodeIdx], sb_height)
        assert_equal(len(late_triggers), 1)
        other_trigger = next(iter(late_triggers))
        self.wait_trigger_everywhere(other_trigger)
        assert_equal(self.trigger_addresses(other_trigger), {self.proposal_address[p_a], self.proposal_address[p_b]})
        assert_equal(self.own_vote(waiting, other_trigger), None)

        while self.nodes[0].getblockcount() + 1 + NO_VOTE_DEADLINE_BLOCKS < sb_height:
            assert self.payee(self.nodes[0].getblockcount() + 2) is not waiting
            self.mine()
            assert_equal(self.own_vote(waiting, other_trigger), None)
            assert_equal(self.own_vote(creator, other_trigger), "no")
        assert_equal(self.nodes[0].getblockcount(), sb_height - NO_VOTE_DEADLINE_BLOCKS - 1)
        self.mine()
        assert_equal(self.own_vote(waiting, other_trigger), "no")
        assert_equal(self.own_vote(waiting, trigger), "yes")


if __name__ == '__main__':
    GovernanceTriggerVotingTest().main()
