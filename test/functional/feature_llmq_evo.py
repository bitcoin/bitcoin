#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_evo.py

Checks EvoNodes

'''
from _decimal import Decimal
from io import BytesIO

from test_framework.mininode import P2PInterface
from test_framework.messages import CBlock, CBlockHeader, CCbTx, CMerkleBlock, FromHex, hash256, msg_getmnlistd, \
    QuorumId, ser_uint256
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal, p2p_port, wait_until
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
        return wait_until(received_mnlistdiff, timeout=timeout)

    def getmnlistdiff(self, baseBlockHash, blockHash):
        msg = msg_getmnlistd(baseBlockHash, blockHash)
        self.last_mnlistdiff = None
        self.send_message(msg)
        self.wait_for_mnlistdiff()
        return self.last_mnlistdiff

class LLMQEvoNodesTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(5, 4, fast_dip3_enforcement=True, evo_count=7)
        self.set_dash_llmq_test_params(4, 4)

    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        null_hash = format(0, "064x")

        for i in range(len(self.nodes)):
            if i != 0:
                self.connect_nodes(i, 0)

        self.activate_dip8()

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        expectedUpdated = [mn.proTxHash for mn in self.mninfo]
        b_0 = self.nodes[0].getbestblockhash()
        self.test_getmnlistdiff(null_hash, b_0, {}, [], expectedUpdated)

        self.mine_quorum(llmq_type_name='llmq_test', llmq_type=100)

        self.log.info("Test that EvoNodes registration is rejected before v19")
        self.test_evo_is_rejected_before_v19()

        self.test_masternode_count(expected_mns_count=4, expected_evo_count=0)

        self.activate_v19(expected_activation_height=900)
        self.log.info("Activated v19 at height:" + str(self.nodes[0].getblockcount()))

        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        (quorum_info_i_0, quorum_info_i_1) = self.mine_cycle_quorum(llmq_type_name='llmq_test_dip0024', llmq_type=103)

        evo_protxhash_list = list()
        for i in range(5):
            evo_info = self.dynamically_add_masternode(evo=True)
            evo_protxhash_list.append(evo_info.proTxHash)
            self.nodes[0].generate(8)
            self.sync_blocks(self.nodes)

            expectedUpdated.append(evo_info.proTxHash)
            b_i = self.nodes[0].getbestblockhash()
            self.test_getmnlistdiff(null_hash, b_i, {}, [], expectedUpdated)

            self.test_masternode_count(expected_mns_count=4, expected_evo_count=i+1)
            self.dynamically_evo_update_service(evo_info)

        self.log.info("Test llmq_platform are formed only with EvoNodes")
        for i in range(3):
            quorum_i_hash = self.mine_quorum(llmq_type_name='llmq_test_platform', llmq_type=106, expected_connections=2, expected_members=3, expected_contributions=3, expected_complaints=0, expected_justifications=0, expected_commitments=3 )
            self.test_quorum_members_are_evo_nodes(quorum_i_hash, llmq_type=106)

        self.log.info("Test that EvoNodes are present in MN list")
        self.test_evo_protx_are_in_mnlist(evo_protxhash_list)

        self.log.info("Test that EvoNodes are paid 4x blocks in a row")
        self.test_evo_payments(window_analysis=256)

        self.activate_v20()
        self.activate_mn_rr()
        self.log.info("Activated MN RewardReallocation at height:" + str(self.nodes[0].getblockcount()))

        # Generate a few blocks to make EvoNode/MN analysis on a pure MN RewardReallocation window
        self.bump_mocktime(1)
        self.nodes[0].generate(4)
        self.sync_blocks()

        self.log.info("Test that EvoNodes are paid 1 block in a row after MN RewardReallocation activation")
        self.test_evo_payments(window_analysis=256, v20active=True)

        self.log.info(self.nodes[0].masternodelist())

        return

    def test_evo_payments(self, window_analysis, v20active=False):
        current_evo = None
        consecutive_payments = 0
        n_payments = 0 if v20active else 4
        for i in range(0, window_analysis):
            payee = self.get_mn_payee_for_block(self.nodes[0].getbestblockhash())
            if payee is not None and payee.evo:
                if current_evo is not None and payee.proTxHash == current_evo.proTxHash:
                    # same EvoNode
                    assert consecutive_payments > 0
                    if not v20active:
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
                    new_payment_value = 0 if v20active else 1
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

            self.nodes[0].generate(1)
            if i % 8 == 0:
                self.sync_blocks()

    def get_mn_payee_for_block(self, block_hash):
        mn_payee_info = self.nodes[0].masternode("payments", block_hash)[0]
        mn_payee_protx = mn_payee_info['masternodes'][0]['proTxHash']

        mninfos_online = self.mninfo.copy()
        for mn_info in mninfos_online:
            if mn_info.proTxHash == mn_payee_protx:
                return mn_info
        return None

    def test_quorum_members_are_evo_nodes(self, quorum_hash, llmq_type):
        quorum_info = self.nodes[0].quorum("info", llmq_type, quorum_hash)
        quorum_members = extract_quorum_members(quorum_info)
        mninfos_online = self.mninfo.copy()
        for qm in quorum_members:
            found = False
            for mn in mninfos_online:
                if mn.proTxHash == qm:
                    assert_equal(mn.evo, True)
                    found = True
                    break
            assert_equal(found, True)

    def test_evo_protx_are_in_mnlist(self, evo_protx_list):
        mn_list = self.nodes[0].masternodelist()
        for evo_protx in evo_protx_list:
            found = False
            for mn in mn_list:
                if mn_list.get(mn)['proTxHash'] == evo_protx:
                    found = True
                    assert_equal(mn_list.get(mn)['type'], "Evo")
            assert_equal(found, True)

    def test_evo_is_rejected_before_v19(self):
        bls = self.nodes[0].bls('generate')
        collateral_address = self.nodes[0].getnewaddress()
        funds_address = self.nodes[0].getnewaddress()
        owner_address = self.nodes[0].getnewaddress()
        voting_address = self.nodes[0].getnewaddress()
        reward_address = self.nodes[0].getnewaddress()

        collateral_amount = 4000
        outputs = {collateral_address: collateral_amount, funds_address: 1}
        collateral_txid = self.nodes[0].sendmany("", outputs)
        self.wait_for_instantlock(collateral_txid, self.nodes[0])
        tip = self.nodes[0].generate(1)[0]
        self.sync_all(self.nodes)

        rawtx = self.nodes[0].getrawtransaction(collateral_txid, 1, tip)
        assert_equal(rawtx['confirmations'], 1)
        collateral_vout = 0
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(collateral_amount):
                collateral_vout = txout['n']
                break
        assert collateral_vout is not None

        ipAndPort = '127.0.0.1:%d' % p2p_port(len(self.nodes))
        operatorReward = len(self.nodes)

        try:
            self.nodes[0].protx('register_evo', collateral_txid, collateral_vout, ipAndPort, owner_address, bls['public'], voting_address, operatorReward, reward_address, funds_address, True)
            # this should never succeed
            assert False
        except:
            self.log.info("protx_evo rejected")

    def test_masternode_count(self, expected_mns_count, expected_evo_count):
        mn_count = self.nodes[0].masternode('count')
        assert_equal(mn_count['total'], expected_mns_count + expected_evo_count)
        detailed_count = mn_count['detailed']
        assert_equal(detailed_count['regular']['total'], expected_mns_count)
        assert_equal(detailed_count['evo']['total'], expected_evo_count)

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
        header = FromHex(CBlockHeader(), hexstr)

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
