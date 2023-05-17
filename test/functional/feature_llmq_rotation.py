#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_rotation.py

Checks LLMQs Quorum Rotation

'''
from io import BytesIO

from test_framework.test_framework import DashTestFramework
from test_framework.messages import CBlock, CBlockHeader, CCbTx, CMerkleBlock, FromHex, hash256, msg_getmnlistd, QuorumId
from test_framework.mininode import P2PInterface
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    wait_until,
)


def intersection(lst1, lst2):
    lst3 = [value for value in lst1 if value in lst2]
    return lst3


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

class LLMQQuorumRotationTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(9, 8, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(4, 4)

    def run_test(self):
        llmq_type=103
        llmq_type_name="llmq_test_dip0024"

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())

        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        for i in range(len(self.nodes)):
            if i != 1:
                self.connect_nodes(i, 0)

        self.activate_dip8()

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        b_h_0 = self.nodes[0].getbestblockhash()

        #Mine 2 quorums so that Chainlocks can be available: Need them to include CL in CbTx as soon as v20 activates
        self.log.info("Mining 2 quorums")
        h_0 = self.mine_quorum()
        h_100_0 = QuorumId(100, int(h_0, 16))
        h_106_0 = QuorumId(106, int(h_0, 16))
        h_104_0 = QuorumId(104, int(h_0, 16))
        h_1 = self.mine_quorum()
        h_100_1 = QuorumId(100, int(h_1, 16))
        h_106_1 = QuorumId(106, int(h_1, 16))
        h_104_1 = QuorumId(104, int(h_1, 16))

        self.log.info("Mine single block, wait for chainlock")
        self.nodes[0].generate(1)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        b_h_1 = self.nodes[0].getbestblockhash()

        expectedDeleted = []
        expectedNew = [h_100_0, h_106_0, h_104_0, h_100_1, h_106_1, h_104_1]
        quorumList = self.test_getmnlistdiff_quorums(b_h_0, b_h_1, {}, expectedDeleted, expectedNew)

        self.activate_v20(expected_activation_height=1440)
        self.log.info("Activated v20 at height:" + str(self.nodes[0].getblockcount()))

        # v20 is active for the next block, not for the tip
        self.nodes[0].generate(1)

        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        #At this point, we need to move forward 3 cycles (3 x 24 blocks) so the first 3 quarters can be created (without DKG sessions)
        #self.log.info("Start at H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))
        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        b_0 = self.nodes[0].getbestblockhash()

        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        (quorum_info_0_0, quorum_info_0_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)
        assert(self.test_quorum_listextended(quorum_info_0_0, llmq_type_name))
        assert(self.test_quorum_listextended(quorum_info_0_1, llmq_type_name))
        quorum_members_0_0 = extract_quorum_members(quorum_info_0_0)
        quorum_members_0_1 = extract_quorum_members(quorum_info_0_1)
        assert_equal(len(intersection(quorum_members_0_0, quorum_members_0_1)), 0)
        self.log.info("Quorum #0_0 members: " + str(quorum_members_0_0))
        self.log.info("Quorum #0_1 members: " + str(quorum_members_0_1))

        q_100_0 = QuorumId(100, int(quorum_info_0_0["quorumHash"], 16))
        q_102_0 = QuorumId(102, int(quorum_info_0_0["quorumHash"], 16))
        q_104_0 = QuorumId(104, int(quorum_info_0_0["quorumHash"], 16))
        q_103_0_0 = QuorumId(103, int(quorum_info_0_0["quorumHash"], 16))
        q_103_0_1 = QuorumId(103, int(quorum_info_0_1["quorumHash"], 16))

        b_1 = self.nodes[0].getbestblockhash()
        expectedDeleted = [h_100_0, h_104_0]
        expectedNew = [q_100_0, q_102_0, q_104_0, q_103_0_0, q_103_0_1]
        quorumList = self.test_getmnlistdiff_quorums(b_0, b_1, quorumList, expectedDeleted, expectedNew)

        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        (quorum_info_1_0, quorum_info_1_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)
        assert(self.test_quorum_listextended(quorum_info_1_0, llmq_type_name))
        assert(self.test_quorum_listextended(quorum_info_1_1, llmq_type_name))
        quorum_members_1_0 = extract_quorum_members(quorum_info_1_0)
        quorum_members_1_1 = extract_quorum_members(quorum_info_1_1)
        assert_equal(len(intersection(quorum_members_1_0, quorum_members_1_1)), 0)
        self.log.info("Quorum #1_0 members: " + str(quorum_members_1_0))
        self.log.info("Quorum #1_1 members: " + str(quorum_members_1_1))

        q_100_1 = QuorumId(100, int(quorum_info_1_0["quorumHash"], 16))
        q_102_1 = QuorumId(102, int(quorum_info_1_0["quorumHash"], 16))
        q_103_1_0 = QuorumId(103, int(quorum_info_1_0["quorumHash"], 16))
        q_103_1_1 = QuorumId(103, int(quorum_info_1_1["quorumHash"], 16))

        b_2 = self.nodes[0].getbestblockhash()
        expectedDeleted = [h_100_1, q_103_0_0, q_103_0_1]
        expectedNew = [q_100_1, q_102_1, q_103_1_0, q_103_1_1]
        quorumList = self.test_getmnlistdiff_quorums(b_1, b_2, quorumList, expectedDeleted, expectedNew)

        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]
        self.sync_blocks(nodes)
        quorum_list = self.nodes[0].quorum("list", llmq_type)
        quorum_blockhash = self.nodes[0].getbestblockhash()
        fallback_blockhash = self.nodes[0].generate(1)[0]
        self.log.info("h("+str(self.nodes[0].getblockcount())+") quorum_list:"+str(quorum_list))

        assert_greater_than_or_equal(len(intersection(quorum_members_0_0, quorum_members_1_0)), 3)
        assert_greater_than_or_equal(len(intersection(quorum_members_0_1, quorum_members_1_1)), 3)

        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        self.log.info("Mine a quorum to invalidate")
        (quorum_info_3_0, quorum_info_3_1) = self.mine_cycle_quorum(llmq_type_name=llmq_type_name, llmq_type=llmq_type)

        new_quorum_list = self.nodes[0].quorum("list", llmq_type)
        assert_equal(len(new_quorum_list[llmq_type_name]), len(quorum_list[llmq_type_name]) + 2)
        new_quorum_blockhash = self.nodes[0].getbestblockhash()
        self.log.info("h("+str(self.nodes[0].getblockcount())+") new_quorum_blockhash:"+new_quorum_blockhash)
        self.log.info("h("+str(self.nodes[0].getblockcount())+") new_quorum_list:"+str(new_quorum_list))
        assert new_quorum_list != quorum_list

        self.log.info("Invalidate the quorum")
        self.bump_mocktime(5)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.nodes[0].invalidateblock(fallback_blockhash)
        assert_equal(self.nodes[0].getbestblockhash(), quorum_blockhash)
        assert_equal(self.nodes[0].quorum("list", llmq_type), quorum_list)

        self.log.info("Reconsider the quorum")
        self.bump_mocktime(5)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        self.nodes[0].reconsiderblock(fallback_blockhash)
        wait_until(lambda: self.nodes[0].getbestblockhash() == new_quorum_blockhash, sleep=1)
        assert_equal(self.nodes[0].quorum("list", llmq_type), new_quorum_list)

    def test_getmnlistdiff_quorums(self, baseBlockHash, blockHash, baseQuorumList, expectedDeleted, expectedNew):
        d = self.test_getmnlistdiff_base(baseBlockHash, blockHash)

        assert_equal(set(d.deletedQuorums), set(expectedDeleted))
        assert_equal(set([QuorumId(e.llmqType, e.quorumHash) for e in d.newQuorums]), set(expectedNew))

        newQuorumList = baseQuorumList.copy()

        for e in d.deletedQuorums:
            newQuorumList.pop(e)

        for e in d.newQuorums:
            newQuorumList[QuorumId(e.llmqType, e.quorumHash)] = e

        cbtx = CCbTx()
        cbtx.deserialize(BytesIO(d.cbTx.vExtraPayload))

        if cbtx.version >= 2:
            hashes = []
            for qc in newQuorumList.values():
                hashes.append(hash256(qc.serialize()))
            hashes.sort()
            merkleRoot = CBlock.get_merkle_root(hashes)
            assert_equal(merkleRoot, cbtx.merkleRootQuorums)

        return newQuorumList


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

    def test_quorum_listextended(self, quorum_info, llmq_type_name):
        extended_quorum_list = self.nodes[0].quorum("listextended")[llmq_type_name]
        quorum_dict = {}
        for dictionary in extended_quorum_list:
            quorum_dict.update(dictionary)
        if quorum_info["quorumHash"] in quorum_dict:
            q = quorum_dict[quorum_info["quorumHash"]]
            if q["minedBlockHash"] != quorum_info["minedBlock"]:
                return False
            if q["creationHeight"] != quorum_info["height"]:
                return False
            if q["quorumIndex"] != quorum_info["quorumIndex"]:
                return False
            return True
        return False

if __name__ == '__main__':
    LLMQQuorumRotationTest().main()
