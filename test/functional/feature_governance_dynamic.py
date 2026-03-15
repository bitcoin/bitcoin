#!/usr/bin/env python3
# Copyright (c) 2018-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests around Syscoin governance."""

import json
import time
import shutil
from decimal import Decimal
from collections import defaultdict
from test_framework.test_framework import DashTestFramework, initialize_datadir
from test_framework.util import assert_equal, satoshi_round, force_finish_mnsync, wait_until_helper_internal
GOVERNANCE_DELETION_DELAY = 10 * 60
SUPERBLOCK_PAYMENT_LIMIT_UP = Decimal('10')
SUPERBLOCK_PAYMENT_LIMIT_DOWN = Decimal('-10')
SUPERBLOCK_PAYMENT_LIMIT_SAME = Decimal('0')
GOVERNANCE_FEE_CONFIRMATIONS = 6
MASTERNODE_SYNC_TICK_SECONDS = 6
MAX_GOVERNANCE_BUDGET = Decimal('5000000.00000000') 
PROPOSAL_END_EPOCH = 60
class SyscoinGovernanceTest(DashTestFramework):
    def set_test_params(self):
        # Using adjusted v20 deployment params to test an edge case where superblock maturity window is equal to deployment window size
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def add_options(self, parser):
        self.add_wallet_options(parser)

    def prepare_object(self, object_type, parent_hash, creation_time, expiry_time, revision, name, amount, payment_address):
        proposal_rev = revision
        proposal_time = int(creation_time)
        proposal_template = {
            "type": object_type,
            "name": name,
            "start_epoch": proposal_time,
            "end_epoch": expiry_time,
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
        assert_equal(self.nodes[0].getsuperblockbudget(), satoshi_round(self.expected_budget))

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
        governance_info = self.nodes[0].getgovernanceinfo()
        sb_cycle = governance_info['superblockcycle']
        self.sb_maturity_window = governance_info['superblockmaturitywindow']
        self.sb_immaturity_window = sb_cycle - self.sb_maturity_window
        self.p0_payout_address = self.nodes[0].getnewaddress()
        self.p1_payout_address = self.nodes[0].getnewaddress()
        self.p2_payout_address = self.nodes[0].getnewaddress()
        self.p3_payout_address = self.nodes[0].getnewaddress()
        self.initial_budget = Decimal('2000000.00000000')
        self.expected_budget = self.initial_budget

        # Ensure nodes are connected at the beginning
        for idx, node_outer in enumerate(self.nodes):
            for idx, node_inner in enumerate(self.nodes):
                if node_inner.index != node_outer.index:
                    self.connect_nodes(node_inner.index, node_outer.index, wait_for_connect=False)

        # Step 1: SB1 - Proposals to push budget up by 10%
        self.log.info("SB1 - Proposals to increase budget by 10%")
        proposals_data = [
            {"amount": self.expected_budget * Decimal(0.5), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal(0.3), "address": self.p1_payout_address},
            {"amount": self.expected_budget * Decimal(0.3), "address": self.p2_payout_address}  # Total: 1.1 times the budget
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)

        # Step 1: SB2 - Proposals to push budget up by 5%
        self.log.info("SB2 - Proposals to increase budget by 5%")
        # expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))  # delete prev proposals and advance ProcessTick
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.51'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.32'), "address": self.p1_payout_address},
            {"amount": self.expected_budget * Decimal('0.22'), "address": self.p2_payout_address}  # Total: 1.05 times the budget
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)
        
        # Step 3: SB3 - Proposals to change proposals budget up by 4% and not see a change in the limit 
        self.log.info("SB3 - Proposals to increase budget by 4%")
        # expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))  # delete prev proposals and advance ProcessTick
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.52'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.29'), "address": self.p1_payout_address},
            {"amount": self.expected_budget * Decimal('0.23'), "address": self.p2_payout_address}  # Total: 1.04 times the budget
        ]   
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)
        
        # Step 4: SB4 - Proposals to decrease budget by 10%
        self.log.info("SB4 - Proposals to decrease budget by 10%")
        # expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))  # delete prev proposals and advance ProcessTick
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])   
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.5'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.2'), "address": self.p1_payout_address},
            {"amount": self.expected_budget * Decimal('0.24'), "address": self.p2_payout_address}  # Total: 0.94 times the budget
        ]   
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        
        self.mine_superblock_and_check_budget(proposals_data)

        # Step 5: SB5 - Proposals to change proposals budget down by 4% and not see a change in the limit
        self.log.info("SB5 - Proposals to decrease budget by 4%")
        # expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))  # delete prev proposals and advance ProcessTick
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.43'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.53'), "address": self.p1_payout_address}  # Total: 0.96 times the budget
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)

        # Step 6: SB6 - Proposals to decrease budget by 5%
        self.log.info("SB6 - Proposals to decrease budget by 5%")
        # expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))  # delete prev proposals and advance ProcessTick
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        # empty SB shouldn't reset the budget
        # Expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))
        for node in self.nodes:
            force_finish_mnsync(node)
        self.mine_superblock_and_check_budget([])
        # Step 7: SB7 - Reindex and verify budget after reindex
        self.log.info(f"SB7 - Reindex and check budget {satoshi_round(self.expected_budget)}")
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.1'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.7'), "address": self.p1_payout_address},
            {"amount": self.expected_budget * Decimal('0.15'), "address": self.p3_payout_address}  # Total: 0.95 times the budget
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)
        # previous proposals were against the higher budget so the limit should increase again with the same proposals
        self.reindex_node_and_check_budget(proposals_data)

        # Step 8: Push budget up to the max cap of 5M
        self.log.info("Pushing budget up to max 5M")
        count = 0
        while count < 7:
            count = count + 1
            proposals_data = [
                {"amount": self.expected_budget * Decimal('0.5'), "address": self.p0_payout_address},
                {"amount": self.expected_budget * Decimal('0.3'), "address": self.p1_payout_address},
                {"amount": self.expected_budget * Decimal('0.3'), "address": self.p2_payout_address}
            ]
            proposals_data = self.prepare_and_submit_proposals(proposals_data)
            self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
            self.mine_superblock_and_check_budget(proposals_data)

        # Step 9: Verify budget cannot exceed max 5M
        self.log.info("Ensuring budget does not exceed 5M")
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.5'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.3'), "address": self.p1_payout_address},
            {"amount": self.expected_budget * Decimal('0.3'), "address": self.p2_payout_address}
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)  # Should still be 5M

        # Step 10: Reduce budget by 10%
        self.log.info("Reducing budget by 10%")
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.4'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.4'), "address": self.p1_payout_address}
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)

        # Step 11: Verify budget cannot go back over 5M
        self.log.info("Ensuring budget does not exceed 5M after decrease")
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.7'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.4'), "address": self.p1_payout_address}
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)
        proposals_data = [
            {"amount": self.expected_budget * Decimal('0.7'), "address": self.p0_payout_address},
            {"amount": self.expected_budget * Decimal('0.4'), "address": self.p1_payout_address}
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)
        proposals_data = [
            {"amount": self.expected_budget, "address": self.p0_payout_address},
            {"amount": Decimal('1'), "address": self.p1_payout_address}
        ]
        proposals_data = self.prepare_and_submit_proposals(proposals_data)
        self.vote_on_proposals([p["hash"] for p in proposals_data], map_vote_signals, map_vote_outcomes)
        self.mine_superblock_and_check_budget(proposals_data)
        # Step 12: Final verification, resync and check budget
        self.log.info(f"Resync and ensure budget consistency {satoshi_round(self.expected_budget)}")
        self.resync_node_and_check_budget(proposals_data)

    def prepare_and_submit_proposals(self, proposals):
        # Expire previous proposals
        self.bump_mocktime(int(PROPOSAL_END_EPOCH))
        self.generate(self.nodes[0], 5)
        self.bump_mocktime(int(GOVERNANCE_DELETION_DELAY + MASTERNODE_SYNC_TICK_SECONDS))
        for node in self.nodes:
            force_finish_mnsync(node)

        proposal_time = self.mocktime
        proposals_data = []

        # Prepare proposals first
        for i, proposal in enumerate(proposals):
            amount = satoshi_round(proposal["amount"])
            address = proposal["address"]
            expiry_time = proposal_time + PROPOSAL_END_EPOCH
            proposal_data = self.prepare_object(1, "%064x" % 0, proposal_time, expiry_time, 1, f"Proposal_{i}", amount, address)
            proposals_data.append({
                "amount": amount,
                "address": address,
                "hex": proposal_data["hex"],
                "collateralHash": proposal_data["collateralHash"],
                "expiry_time": proposal.get("expiry_time")
            })

        self.generate(self.nodes[0], GOVERNANCE_FEE_CONFIRMATIONS, sync_fun=self.no_op)
        self.sync_blocks()

        # Submit proposals and store returned hashes explicitly
        for i, proposal in enumerate(proposals_data):
            proposal_hash = self.nodes[0].gobject_submit("0", 1, proposal_time, proposal["hex"], proposal["collateralHash"])
            proposal["hash"] = proposal_hash
            self.log.info(f"Submitted proposal {i} with hash {proposal_hash}")

        return proposals_data


    def vote_on_proposals(self, proposal_hashes, map_vote_signals, map_vote_outcomes):
        self.log.info(f"Voting on proposals: {proposal_hashes}")  # Debug statement
        self.sync_gobject_list(len(proposal_hashes))  # Ensure all nodes have the proposals
        for proposal_hash in proposal_hashes:
            self.nodes[0].gobject_vote_many(proposal_hash, map_vote_signals[1], map_vote_outcomes[1])

    def sync_gobject_list(self, expected_count=4):
        # Ensures that each node has processed all governance objects
        def check_each_node_has_gobjects(node, expected_count=expected_count):
            self.bump_mocktime(1)
            return len(node.gobject_list("valid", "proposals")) == expected_count

        # Check for each node individually
        for i in range(len(self.nodes)):
            self.wait_until(lambda: check_each_node_has_gobjects(self.nodes[i]), timeout=10)
        
        # Double-check that all nodes are synchronized
        self.wait_until(lambda: all(len(node.gobject_list("valid", "proposals")) > 0 for node in self.nodes), timeout=10)

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
        
    def mine_superblock_and_check_budget(self, proposals_data):
        # Check expected budget before mining
        actual_budget = self.nodes[0].getsuperblockbudget()
        block_count = self.nodes[0].getblockcount()
        gov_info = self.nodes[0].getgovernanceinfo()
        sb_cycle = gov_info['superblockcycle']
        sb_height = gov_info['nextsuperblock']

        # Move until 1 block before the Superblock maturity window starts
        n = self.sb_immaturity_window - block_count % sb_cycle
        self.generate(self.nodes[0], n - 1)
        self.log.info(f"Waiting for trigger")
        if len(proposals_data) > 0:
            self.wait_for_trigger(sb_height)

        block_count = self.nodes[0].getblockcount()
        n = sb_height - block_count
        if n > 0:
            self.generate(self.nodes[0], n)

        self.log.info(f"Mined superblock at height {sb_height}")

        total_funded_amount = self.verify_proposals_in_superblock(proposals_data)
        funded_percentage = (total_funded_amount / actual_budget) * Decimal('100.0')

        if funded_percentage >= Decimal('100') + SUPERBLOCK_PAYMENT_LIMIT_UP / Decimal('2'):
            self.expected_budget = min(
                actual_budget * (Decimal('1') + SUPERBLOCK_PAYMENT_LIMIT_UP / Decimal('100')), MAX_GOVERNANCE_BUDGET
            )
        elif funded_percentage > Decimal('0') and funded_percentage <= Decimal('100') + SUPERBLOCK_PAYMENT_LIMIT_DOWN / Decimal('2'):
            self.expected_budget = actual_budget * (Decimal('1') + SUPERBLOCK_PAYMENT_LIMIT_DOWN / Decimal('100'))
        else:
            self.expected_budget = actual_budget
        self.expected_budget = satoshi_round(self.expected_budget)

        self.log.info(
            f"Next expected budget updated to: {self.expected_budget} based on funded_percentage: {funded_percentage}%"
        )
        self.check_superblockbudget()

    def verify_proposals_in_superblock(self, proposals_data):
        if len(proposals_data) == 0:
            return Decimal('0')
        coinbase_outputs = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]["vout"]

        # Create a set of addresses used in expected proposals
        proposal_addresses = {p["address"] for p in proposals_data}

        # Only count outputs that go to proposal addresses
        included_payments = defaultdict(Decimal)
        for output in coinbase_outputs:
            address = output["scriptPubKey"].get("address")
            if address in proposal_addresses:
                included_payments[address] += Decimal(output["value"])


        total_payment = sum(included_payments.values())
        assert total_payment <= MAX_GOVERNANCE_BUDGET, "Total proposal payments exceed MAX superblock budget"
        return total_payment


    def reindex_node_and_check_budget(self, proposals_data):
        # Check expected budget before mining
        actual_budget = self.nodes[0].getsuperblockbudget()
        block_count = self.nodes[0].getblockcount()
        gov_info = self.nodes[0].getgovernanceinfo()
        sb_cycle = gov_info['superblockcycle']
        sb_height = gov_info['nextsuperblock']

        # Move until 1 block before the Superblock maturity window starts
        n = self.sb_immaturity_window - block_count % sb_cycle
        self.generate(self.nodes[0], n - 1)
        self.sync_blocks()
        self.stop_node(1)
        self.start_node(1, extra_args=["-mocktime=" + str(self.mocktime), '-reindex', *self.extra_args[1]])
        self.nodes[1].setnetworkactive(True)
        for idx, node_outer in enumerate(self.nodes):
            for idx, node_inner in enumerate(self.nodes):
                if node_inner.index != node_outer.index:
                    self.connect_nodes(node_inner.index, node_outer.index, wait_for_connect=False)

        self.sync_mnsync([self.nodes[1]])
        self.log.info(f"Waiting for trigger")
        self.wait_for_trigger(sb_height)
        block_count = self.nodes[0].getblockcount()
        n = sb_height - block_count
        if n > 0:
            self.generate(self.nodes[0], n)
        self.log.info(f"Mined superblock at height {sb_height}")
        total_funded_amount = self.verify_proposals_in_superblock(proposals_data)

        funded_percentage = (total_funded_amount / actual_budget) * Decimal('100.0')

        if funded_percentage >= Decimal('100') + SUPERBLOCK_PAYMENT_LIMIT_UP / Decimal('2'):
            self.expected_budget = min(
                actual_budget * (Decimal('1') + SUPERBLOCK_PAYMENT_LIMIT_UP / Decimal('100')), MAX_GOVERNANCE_BUDGET
            )
        elif funded_percentage > Decimal('0') and funded_percentage <= Decimal('100') + SUPERBLOCK_PAYMENT_LIMIT_DOWN / Decimal('2'):
            self.expected_budget = actual_budget * (Decimal('1') + SUPERBLOCK_PAYMENT_LIMIT_DOWN / Decimal('100'))
        else:
            self.expected_budget = actual_budget
        self.expected_budget = satoshi_round(self.expected_budget)
        self.log.info(
            f"Next expected budget updated to: {self.expected_budget} based on funded_percentage: {funded_percentage}%"
        )
        self.check_superblockbudget()

    def resync_node_and_check_budget(self, proposals_data):
        # Check expected budget before mining
        actual_budget = self.nodes[0].getsuperblockbudget()
        block_count = self.nodes[0].getblockcount()
        gov_info = self.nodes[0].getgovernanceinfo()
        sb_cycle = gov_info['superblockcycle']
        sb_height = gov_info['nextsuperblock']

        # Move until 1 block before the Superblock maturity window starts
        n = self.sb_immaturity_window - block_count % sb_cycle
        self.generate(self.nodes[0], n - 1)
        self.sync_blocks()
        self.stop_node(1)
        shutil.rmtree(self.nodes[1].datadir_path)
        initialize_datadir(self.options.tmpdir, 1, self.chain)
        self.start_node(1, extra_args=["-mocktime=" + str(self.mocktime), '-networkactive=0', *self.extra_args[1]])
        self.nodes[1].setnetworkactive(True)
        for idx, node_outer in enumerate(self.nodes):
            for idx, node_inner in enumerate(self.nodes):
                if node_inner.index != node_outer.index:
                    self.connect_nodes(node_inner.index, node_outer.index, wait_for_connect=False)

        self.sync_mnsync([self.nodes[1]])
        self.log.info(f"Waiting for trigger")
        self.wait_for_trigger(sb_height)
        block_count = self.nodes[0].getblockcount()
        n = sb_height - block_count
        if n > 0:
            self.generate(self.nodes[0], n)
        self.log.info(f"Mined superblock at height {sb_height}")
        total_funded_amount = self.verify_proposals_in_superblock(proposals_data)

        funded_percentage = (total_funded_amount / actual_budget) * Decimal('100.0')

        if funded_percentage >= Decimal('100') + SUPERBLOCK_PAYMENT_LIMIT_UP / Decimal('2'):
            self.expected_budget = min(
                actual_budget * (Decimal('1') + SUPERBLOCK_PAYMENT_LIMIT_UP / Decimal('100')), MAX_GOVERNANCE_BUDGET
            )
        elif funded_percentage > Decimal('0') and funded_percentage <= Decimal('100') + SUPERBLOCK_PAYMENT_LIMIT_DOWN / Decimal('2'):
            self.expected_budget = actual_budget * (Decimal('1') + SUPERBLOCK_PAYMENT_LIMIT_DOWN / Decimal('100'))
        else:
            self.expected_budget = actual_budget
        self.expected_budget = satoshi_round(self.expected_budget)
        self.log.info(
            f"Next expected budget updated to: {self.expected_budget} based on funded_percentage: {funded_percentage}%"
        )
        self.check_superblockbudget()
if __name__ == '__main__':
    SyscoinGovernanceTest().main()
