#!/usr/bin/env python3
# Copyright (c) 2015-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_evo.py

Checks EvoNodes

'''
from io import BytesIO
from typing import Optional

from test_framework.p2p import P2PInterface
from test_framework.messages import CBlock, CBlockHeader, CCbTx, CMerkleBlock, from_hex, hash256, msg_getmnlistd, \
    QuorumId, ser_uint256
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import (
    assert_equal, assert_greater_than_or_equal,
)


def extract_quorum_members(quorum_info):
    return [d['proTxHash'] for d in quorum_info["members"]]

class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.last_mnlistdiff = None

    def on_mnlistdiff(self, message):
        self.last_mnlistdiff = message

    def wait_for_mnlistdiff(self, timeout=30):
        def received_mnlistdiff():
            return self.last_mnlistdiff is not None
        return self.wait_until(received_mnlistdiff, timeout=timeout)

    def getmnlistdiff(self, baseBlockHash, blockHash):
        msg = msg_getmnlistd(baseBlockHash, blockHash)
        self.last_mnlistdiff = None
        self.send_message(msg)
        self.wait_for_mnlistdiff()
        return self.last_mnlistdiff

class LLMQEvoNodesTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, evo_count=5)
        self.mn_rr_height = 400

    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        null_hash = format(0, "064x")

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)

        expectedUpdated = [mn.proTxHash for mn in self.mninfo]
        b_0 = self.nodes[0].getbestblockhash()
        self.test_getmnlistdiff(null_hash, b_0, {}, [], expectedUpdated)

        self.test_masternode_count(expected_mns_count=3, expected_evo_count=0)
        evo_protxhash_list = list()
        for i in range(self.evo_count):
            evo_info: MasternodeInfo = self.dynamically_add_masternode(evo=True)
            evo_protxhash_list.append(evo_info.proTxHash)
            self.generate(self.nodes[0], 8, sync_fun=lambda: self.sync_blocks())

            expectedUpdated.append(evo_info.proTxHash)
            b_i = self.nodes[0].getbestblockhash()
            self.test_getmnlistdiff(null_hash, b_i, {}, [], expectedUpdated)

            self.test_masternode_count(expected_mns_count=3, expected_evo_count=i+1)
            self.dynamically_evo_update_service(evo_info)

        self.log.info("Test llmq_platform are formed only with EvoNodes")
        for _ in range(3):
            quorum_i_hash = self.mine_quorum(llmq_type_name='llmq_test_platform', llmq_type=106)
            self.test_quorum_members_are_evo_nodes(quorum_i_hash, llmq_type=106)

        self.log.info("Test that EvoNodes are present in MN list")
        self.test_evo_protx_are_in_mnlist(evo_protxhash_list)

        self.log.info("Test that EvoNodes are paid 4x blocks in a row")
        self.test_evo_payments(window_analysis=48, mnrr_active=False)
        self.test_masternode_winners()

        self.activate_mn_rr()
        self.log.info("Activated MN RewardReallocation, current height:" + str(self.nodes[0].getblockcount()))

        # Generate a few blocks to make EvoNode/MN analysis on a pure MN RewardReallocation window
        self.bump_mocktime(1)
        self.generate(self.nodes[0], 4, sync_fun=lambda: self.sync_blocks())

        self.log.info("Test that EvoNodes are paid 1 block in a row after MN RewardReallocation activation")
        self.test_evo_payments(window_analysis=48, mnrr_active=True)
        self.test_masternode_winners(mn_rr_active=True)

    def test_evo_payments(self, window_analysis, mnrr_active):
        current_evo: MasternodeInfo = None
        consecutive_payments = 0
        n_payments = 0 if mnrr_active else 4
        for i in range(0, window_analysis):
            payee: MasternodeInfo = self.get_mn_payee_for_block(self.nodes[0].getbestblockhash())
            if payee is not None and payee.evo:
                if current_evo is not None and payee.proTxHash == current_evo.proTxHash:
                    # same EvoNode
                    assert consecutive_payments > 0
                    if not mnrr_active:
                        consecutive_payments += 1
                    consecutive_payments_rpc = self.nodes[0].protx('info', current_evo.proTxHash)['state']['consecutivePayments']
                    assert_equal(consecutive_payments, consecutive_payments_rpc)
                else:
                    # new EvoNode
                    if current_evo is not None:
                        # make sure the old one was paid N times in a row
                        assert_equal(consecutive_payments, n_payments)
                        consecutive_payments_rpc = self.nodes[0].protx('info', current_evo.proTxHash)['state']['consecutivePayments']
                        # old EvoNode should have its nConsecutivePayments reset to 0
                        assert_equal(consecutive_payments_rpc, 0)
                    consecutive_payments_rpc = self.nodes[0].protx('info', payee.proTxHash)['state']['consecutivePayments']
                    # if EvoNode is the one we start "for" loop with,
                    # we have no idea how many times it was paid before - rely on rpc results here
                    new_payment_value = 0 if mnrr_active else 1
                    consecutive_payments = consecutive_payments_rpc if i == 0 and current_evo is None else new_payment_value
                    current_evo = payee
                    assert_equal(consecutive_payments, consecutive_payments_rpc)
            else:
                # not an EvoNode
                if current_evo is not None:
                    # make sure the old one was paid N times in a row
                    assert_equal(consecutive_payments, n_payments)
                    consecutive_payments_rpc = self.nodes[0].protx('info', current_evo.proTxHash)['state']['consecutivePayments']
                    # old EvoNode should have its nConsecutivePayments reset to 0
                    assert_equal(consecutive_payments_rpc, 0)
                current_evo = None
                consecutive_payments = 0

            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            if i % 8 == 0:
                self.sync_blocks()

    def get_mn_payee_for_block(self, block_hash) -> Optional[MasternodeInfo]:
        mn_payee_info = self.nodes[0].masternode("payments", block_hash)[0]
        mn_payee_protx = mn_payee_info['masternodes'][0]['proTxHash']

        mninfos_online = self.mninfo.copy()
        for mn_info in mninfos_online: # type: MasternodeInfo
            if mn_info.proTxHash == mn_payee_protx:
                return mn_info
        return None

    def test_quorum_members_are_evo_nodes(self, quorum_hash, llmq_type):
        quorum_info = self.nodes[0].quorum("info", llmq_type, quorum_hash)
        quorum_members = extract_quorum_members(quorum_info)
        mninfos_online = self.mninfo.copy()
        for qm in quorum_members:
            found = False
            for mn in mninfos_online: # type: MasternodeInfo
                if mn.proTxHash == qm:
                    assert_equal(mn.evo, True)
                    found = True
                    break
            assert_equal(found, True)

    def test_evo_protx_are_in_mnlist(self, evo_protx_list):
        mn_list = self.nodes[0].masternodelist()
        mn_list_evo = self.nodes[0].masternodelist(mode="evo")
        for evo_protx in evo_protx_list:
            found_in_mns = False
            for _, mn in mn_list.items():
                if mn['proTxHash'] == evo_protx:
                    found_in_mns = True
                    assert_equal(mn['type'], "Evo")
            assert_equal(found_in_mns, True)

            found_in_evos = False
            for _, mn in mn_list_evo.items():
                assert_equal(mn['type'], "Evo")
                if mn['proTxHash'] == evo_protx:
                    found_in_evos = True
            assert_equal(found_in_evos, True)

    def test_masternode_count(self, expected_mns_count, expected_evo_count):
        mn_count = self.nodes[0].masternode('count')
        assert_equal(mn_count['total'], expected_mns_count + expected_evo_count)
        detailed_count = mn_count['detailed']
        assert_equal(detailed_count['regular']['total'], expected_mns_count)
        assert_equal(detailed_count['evo']['total'], expected_evo_count)

    def test_masternode_winners(self, mn_rr_active=False):
        # ignore recent winners, test future ones only
        # we get up to 21 entries here: tip + up to 20 future payees
        winners = self.nodes[0].masternode('winners', '0')
        weighted_count = self.mn_count + self.evo_count * (1 if mn_rr_active else 4)
        assert_equal(len(winners.keys()) - 1, 20 if weighted_count > 20 else weighted_count)
        consecutive_payments = 0
        full_consecutive_payments_found = 0
        payment_cycles = 0
        first_payee = None
        prev_winner = None
        for height in winners.keys():
            winner = winners[height]
            if mn_rr_active:
                assert_equal(prev_winner == winner, False)
            else:
                if prev_winner == winner:
                    consecutive_payments += 1
                else:
                    if consecutive_payments == 3:
                        full_consecutive_payments_found += 1
                    consecutive_payments = 0
                assert_greater_than_or_equal(3, consecutive_payments)
            if consecutive_payments == 0 and winner == first_payee:
                payment_cycles += 1
            if first_payee is None:
                first_payee = winner
            prev_winner = winner
        if mn_rr_active:
            assert_equal(full_consecutive_payments_found, 0)
        else:
            assert_greater_than_or_equal(full_consecutive_payments_found, (len(winners.keys()) - 1 - self.mn_count) // 4 - 1)
        assert_equal(payment_cycles, (len(winners.keys()) - 1) // weighted_count)

    def test_getmnlistdiff(self, baseBlockHash, blockHash, baseMNList, expectedDeleted, expectedUpdated):
        d = self.test_getmnlistdiff_base(baseBlockHash, blockHash)

        # Assert that the deletedMNs and mnList fields are what we expected
        assert_equal(set(d.deletedMNs), set([int(e, 16) for e in expectedDeleted]))
        assert_equal(set([e.proRegTxHash for e in d.mnList]), set(int(e, 16) for e in expectedUpdated))

        # Build a new list based on the old list and the info from the diff
        newMNList = baseMNList.copy()
        for e in d.deletedMNs:
            newMNList.pop(format(e, '064x'))
        for e in d.mnList:
            newMNList[format(e.proRegTxHash, '064x')] = e

        cbtx = CCbTx()
        cbtx.deserialize(BytesIO(d.cbTx.vExtraPayload))

        # Verify that the merkle root matches what we locally calculate
        hashes = []
        for mn in sorted(newMNList.values(), key=lambda mn: ser_uint256(mn.proRegTxHash)):
            hashes.append(hash256(mn.serialize(with_version = False)))
        merkleRoot = CBlock.get_merkle_root(hashes)
        assert_equal(merkleRoot, cbtx.merkleRootMNList)

        return newMNList

    def test_getmnlistdiff_base(self, baseBlockHash, blockHash):
        hexstr = self.nodes[0].getblockheader(blockHash, False)
        header = from_hex(CBlockHeader(), hexstr)

        d = self.test_node.getmnlistdiff(int(baseBlockHash, 16), int(blockHash, 16))
        assert_equal(d.baseBlockHash, int(baseBlockHash, 16))
        assert_equal(d.blockHash, int(blockHash, 16))

        # Check that the merkle proof is valid
        proof = CMerkleBlock(header, d.merkleProof)
        proof = proof.serialize().hex()
        assert_equal(self.nodes[0].verifytxoutproof(proof), [d.cbTx.hash])

        # Check if P2P messages match with RPCs
        d2 = self.nodes[0].protx("diff", baseBlockHash, blockHash)
        assert_equal(d2["baseBlockHash"], baseBlockHash)
        assert_equal(d2["blockHash"], blockHash)
        assert_equal(d2["cbTxMerkleTree"], d.merkleProof.serialize().hex())
        assert_equal(d2["cbTx"], d.cbTx.serialize().hex())
        assert_equal(set([int(e, 16) for e in d2["deletedMNs"]]), set(d.deletedMNs))
        assert_equal(set([int(e["proRegTxHash"], 16) for e in d2["mnList"]]), set([e.proRegTxHash for e in d.mnList]))
        assert_equal(set([QuorumId(e["llmqType"], int(e["quorumHash"], 16)) for e in d2["deletedQuorums"]]), set(d.deletedQuorums))
        assert_equal(set([QuorumId(e["llmqType"], int(e["quorumHash"], 16)) for e in d2["newQuorums"]]), set([QuorumId(e.llmqType, e.quorumHash) for e in d.newQuorums]))

        return d

if __name__ == '__main__':
    LLMQEvoNodesTest().main()
