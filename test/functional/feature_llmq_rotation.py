#!/usr/bin/env python3
# Copyright (c) 2015-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_rotation.py

Checks LLMQs Quorum Rotation

'''
import struct
from io import BytesIO

from test_framework.test_framework import DashTestFramework
from test_framework.messages import CBlock, CBlockHeader, CCbTx, CMerkleBlock, from_hex, hash256, msg_getmnlistd, QuorumId, ser_uint256, sha256
from test_framework.p2p import P2PInterface
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
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
        return self.wait_until(received_mnlistdiff, timeout=timeout)

    def getmnlistdiff(self, baseBlockHash, blockHash):
        msg = msg_getmnlistd(baseBlockHash, blockHash)
        self.last_mnlistdiff = None
        self.send_message(msg)
        self.wait_for_mnlistdiff()
        return self.last_mnlistdiff

class LLMQQuorumRotationTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(9, 8, extra_args=[["-vbparams=testdummy:999999999999:999999999999"]] * 9)
        self.set_dash_llmq_test_params(4, 4)
        self.delay_v20_and_mn_rr(height=300)

    def run_test(self):
        llmq_type=103
        llmq_type_name="llmq_test_dip0024"

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())

        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        b_h_0 = self.nodes[0].getbestblockhash()

        tip = self.nodes[0].getblockcount()
        next_dkg = 24 - (tip % 24)
        for node in self.nodes:
            dkg_info = node.quorum("dkginfo")
            assert_equal(dkg_info['active_dkgs'], 0)
            assert_equal(dkg_info['next_dkg'], next_dkg)

        #Mine 2 quorums so that Chainlocks can be available: Need them to include CL in CbTx as soon as v20 activates
        self.log.info("Mining 2 quorums")
        h_0 = self.mine_quorum()
        h_100_0 = QuorumId(100, int(h_0, 16))
        h_1 = self.mine_quorum()
        h_100_1 = QuorumId(100, int(h_1, 16))

        self.log.info("Mine single block, wait for chainlock")
        self.generate(self.nodes[0], 1)
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        b_h_1 = self.nodes[0].getbestblockhash()

        tip = self.nodes[0].getblockcount()
        next_dkg = 24 - (tip % 24)
        assert next_dkg < 24
        nonzero_dkgs = 0
        for i in range(len(self.nodes)):
            dkg_info = self.nodes[i].quorum("dkginfo")
            if i == 0:
                assert_equal(dkg_info['active_dkgs'], 0)
            nonzero_dkgs += dkg_info['active_dkgs']
            assert_equal(dkg_info['next_dkg'], next_dkg)
        assert_equal(nonzero_dkgs, 4) # 1 quorums 4 nodes

        expectedDeleted = []
        expectedNew = [h_100_0, h_100_1]
        quorumList = self.test_getmnlistdiff_quorums(b_h_0, b_h_1, {}, expectedDeleted, expectedNew, testQuorumsCLSigs=False)

        self.activate_v20(expected_activation_height=self.mn_rr_height)
        self.log.info(f"Activated v20 at height: {self.nodes[0].getblockcount()}")

        # v20 is active for the next block, not for the tip
        self.generate(self.nodes[0], 1)

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

        # At this point, we want to wait for CLs just before the self.mine_cycle_quorum to diversify the CLs in CbTx.
        # Although because here a new quorum cycle is starting, and we don't want to mine them now, mine 8 blocks (to skip all DKG phases)
        nodes = [self.nodes[0]] + [mn.get_node(self) for mn in self.mninfo.copy()]
        self.generate(self.nodes[0], 8, sync_fun=lambda: self.sync_blocks(nodes))
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        # And for the remaining blocks, enforce new CL in CbTx
        skip_count = 23 - (self.nodes[0].getblockcount() % 24)
        for _ in range(skip_count):
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(nodes))
            self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())


        (quorum_info_0_0, quorum_info_0_1) = self.mine_cycle_quorum(is_first=False)
        assert(self.test_quorum_listextended(quorum_info_0_0, llmq_type_name))
        assert(self.test_quorum_listextended(quorum_info_0_1, llmq_type_name))
        quorum_members_0_0 = extract_quorum_members(quorum_info_0_0)
        quorum_members_0_1 = extract_quorum_members(quorum_info_0_1)
        assert_equal(len(intersection(quorum_members_0_0, quorum_members_0_1)), 0)
        self.log.info("Quorum #0_0 members: " + str(quorum_members_0_0))
        self.log.info("Quorum #0_1 members: " + str(quorum_members_0_1))

        q_100_0 = QuorumId(100, int(quorum_info_0_0["quorumHash"], 16))
        q_103_0_0 = QuorumId(103, int(quorum_info_0_0["quorumHash"], 16))
        q_103_0_1 = QuorumId(103, int(quorum_info_0_1["quorumHash"], 16))

        b_1 = self.nodes[0].getbestblockhash()
        expectedDeleted = [h_100_0]
        expectedNew = [q_100_0, q_103_0_0, q_103_0_1]
        quorumList = self.test_getmnlistdiff_quorums(b_0, b_1, quorumList, expectedDeleted, expectedNew)

        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        (quorum_info_1_0, quorum_info_1_1) = self.mine_cycle_quorum(is_first=False)
        assert(self.test_quorum_listextended(quorum_info_1_0, llmq_type_name))
        assert(self.test_quorum_listextended(quorum_info_1_1, llmq_type_name))
        quorum_members_1_0 = extract_quorum_members(quorum_info_1_0)
        quorum_members_1_1 = extract_quorum_members(quorum_info_1_1)
        assert_equal(len(intersection(quorum_members_1_0, quorum_members_1_1)), 0)
        self.log.info("Quorum #1_0 members: " + str(quorum_members_1_0))
        self.log.info("Quorum #1_1 members: " + str(quorum_members_1_1))

        q_100_1 = QuorumId(100, int(quorum_info_1_0["quorumHash"], 16))
        q_103_1_0 = QuorumId(103, int(quorum_info_1_0["quorumHash"], 16))
        q_103_1_1 = QuorumId(103, int(quorum_info_1_1["quorumHash"], 16))

        b_2 = self.nodes[0].getbestblockhash()
        expectedDeleted = [h_100_1, q_103_0_0, q_103_0_1]
        expectedNew = [q_100_1, q_103_1_0, q_103_1_1]
        quorumList = self.test_getmnlistdiff_quorums(b_1, b_2, quorumList, expectedDeleted, expectedNew)

        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.get_node(self) for mn in mninfos_online]
        self.sync_blocks(nodes)
        quorum_list = self.nodes[0].quorum("list", llmq_type)
        quorum_blockhash = self.nodes[0].getbestblockhash()
        fallback_blockhash = self.generate(self.nodes[0], 1)[0]
        self.log.info("h("+str(self.nodes[0].getblockcount())+") quorum_list:"+str(quorum_list))

        assert_greater_than_or_equal(len(intersection(quorum_members_0_0, quorum_members_1_0)), 3)
        assert_greater_than_or_equal(len(intersection(quorum_members_0_1, quorum_members_1_1)), 3)

        self.log.info("Wait for chainlock")
        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

        self.log.info("Mine a quorum to invalidate")
        (quorum_info_3_0, quorum_info_3_1) = self.mine_cycle_quorum(is_first=False)

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
        self.wait_until(lambda: self.nodes[0].getbestblockhash() == new_quorum_blockhash)
        assert_equal(self.nodes[0].quorum("list", llmq_type), new_quorum_list)

        self.log.info("Test 'quorum rotationinfo' RPC")
        genesis_blockhash = self.nodes[0].getblockhash(0)
        block_count = self.nodes[0].getblockcount()
        hmc_base_blockhash = self.nodes[0].getblockhash(block_count - (block_count % 24) - 24 - 8)
        best_block_hash = self.nodes[0].getbestblockhash()
        rpc_qr_info = self.nodes[0].quorum("rotationinfo", best_block_hash, False, [hmc_base_blockhash])
        assert_equal(rpc_qr_info["mnListDiffTip"]["blockHash"], best_block_hash)
        assert_equal(rpc_qr_info["mnListDiffTip"]["baseBlockHash"], rpc_qr_info["mnListDiffH"]["blockHash"])
        assert_equal(rpc_qr_info["mnListDiffH"]["baseBlockHash"], rpc_qr_info["mnListDiffAtHMinusC"]["blockHash"])
        assert_equal(rpc_qr_info["mnListDiffAtHMinusC"]["blockHash"], hmc_base_blockhash)
        assert_equal(rpc_qr_info["mnListDiffAtHMinusC"]["baseBlockHash"], hmc_base_blockhash)
        assert_equal(rpc_qr_info["mnListDiffAtHMinusC"]["newQuorums"], [])
        assert_equal(rpc_qr_info["mnListDiffAtHMinusC"]["deletedQuorums"], [])
        assert_equal(rpc_qr_info["mnListDiffAtHMinus2C"]["baseBlockHash"], rpc_qr_info["mnListDiffAtHMinus3C"]["blockHash"])
        assert_equal(rpc_qr_info["mnListDiffAtHMinus3C"]["baseBlockHash"], genesis_blockhash)

    def test_getmnlistdiff_quorums(self, baseBlockHash, blockHash, baseQuorumList, expectedDeleted, expectedNew, testQuorumsCLSigs = True):
        d = self.test_getmnlistdiff_base(baseBlockHash, blockHash, testQuorumsCLSigs)

        assert_equal(set(d.deletedQuorums), set(expectedDeleted))
        assert_equal(set([QuorumId(e.llmqType, e.quorumHash) for e in d.newQuorums]), set(expectedNew))

        newQuorumList = baseQuorumList.copy()

        for e in d.deletedQuorums:
            newQuorumList.pop(e)

        for e in d.newQuorums:
            newQuorumList[QuorumId(e.llmqType, e.quorumHash)] = e

        cbtx = CCbTx()
        cbtx.deserialize(BytesIO(d.cbTx.vExtraPayload))

        if cbtx.nVersion >= 2:
            hashes = []
            for qc in newQuorumList.values():
                hashes.append(hash256(qc.serialize()))
            hashes.sort()
            merkleRoot = CBlock.get_merkle_root(hashes)
            assert_equal(merkleRoot, cbtx.merkleRootQuorums)

        return newQuorumList


    def test_getmnlistdiff_base(self, baseBlockHash, blockHash, testQuorumsCLSigs):
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
        # Check if P2P quorumsCLSigs matches with the corresponding in RPC
        rpc_quorums_clsigs_dict = {k: v for d in d2["quorumsCLSigs"] for k, v in d.items()}
        # p2p_quorums_clsigs_dict is constructed from the P2P message so it can be easily compared to rpc_quorums_clsigs_dict
        p2p_quorums_clsigs_dict = dict()
        for key, value in d.quorumsCLSigs.items():
            idx_list = list(value)
            p2p_quorums_clsigs_dict[key.hex()] = idx_list
        assert_equal(rpc_quorums_clsigs_dict, p2p_quorums_clsigs_dict)
        # The following test must be checked only after v20 activation
        if testQuorumsCLSigs:
            # Total number of corresponding quorum indexes in quorumsCLSigs must be equal to the total of quorums in newQuorums
            assert_equal(len(d2["newQuorums"]), sum(len(value) for value in rpc_quorums_clsigs_dict.values()))
            for cl_sig, value in rpc_quorums_clsigs_dict.items():
                for q in value:
                    self.test_verify_quorums(d2["newQuorums"][q], cl_sig)
        return d

    def test_verify_quorums(self, quorum_info, quorum_cl_sig):
        if int(quorum_cl_sig, 16) == 0:
            # Skipping null-CLSig. No need to verify old way of shuffling (using BlockHash)
            return
        if quorum_info["version"] == 2 or quorum_info["version"] == 4:
            # Skipping rotated quorums. Too complicated to implemented.
            # TODO: Implement rotated quorum verification using CLSigs
            return
        quorum_height = self.nodes[0].getblock(quorum_info["quorumHash"])["height"]
        work_height = quorum_height - 8
        modifier = self.get_hash_modifier(quorum_info["llmqType"], work_height, quorum_cl_sig)
        mn_list = self.nodes[0].protx('diff', 1, work_height)["mnList"]
        scored_mns = []
        # Compute each valid mn score and add them (mn, score) in scored_mns
        for mn in mn_list:
            if mn["isValid"] is False:
                # Skip invalid mns
                continue
            score = self.compute_mn_score(mn, modifier)
            scored_mns.append((mn, score))
        # Sort the list based on the score in descending order
        scored_mns.sort(key=lambda x: x[1], reverse=True)
        llmq_size = self.get_llmq_size(int(quorum_info["llmqType"]))
        # Keep the first llmq_size mns
        scored_mns = scored_mns[:llmq_size]
        quorum_info_members = self.nodes[0].quorum('info', quorum_info["llmqType"], quorum_info["quorumHash"])["members"]
        # Make sure that each quorum member returned from quorum info RPC is matched in our scored_mns list
        for m in quorum_info_members:
            found = False
            for e in scored_mns:
                if m["proTxHash"] == e[0]["proRegTxHash"]:
                    found = True
                    break
            assert found
        return

    def get_hash_modifier(self, llmq_type, height, cl_sig):
        bytes = b""
        bytes += struct.pack('<B', int(llmq_type))
        bytes += struct.pack('<i', int(height))
        bytes += bytes.fromhex(cl_sig)
        return hash256(bytes)[::-1].hex()

    def compute_mn_score(self, mn, modifier):
        bytes = b""
        bytes += ser_uint256(int(mn["proRegTxHash"], 16))
        bytes += ser_uint256(int(mn["confirmedHash"], 16))
        confirmed_hash_pro_regtx_hash = sha256(bytes)[::-1].hex()

        bytes_2 = b""
        bytes_2 += ser_uint256(int(confirmed_hash_pro_regtx_hash, 16))
        bytes_2 += ser_uint256(int(modifier, 16))
        score = sha256(bytes_2)[::-1].hex()
        return int(score, 16)

    def get_llmq_size(self, llmq_type):
        return {
            100: 4, # In this test size for llmqType 100 is overwritten to 4
            103: 4,
            106: 3
        }.get(llmq_type, -1)

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

    def move_to_next_cycle(self):
        cycle_length = 24
        mninfos_online = self.mninfo.copy()
        nodes = [self.nodes[0]] + [mn.get_node(self) for mn in mninfos_online]
        cur_block = self.nodes[0].getblockcount()

        # move forward to next DKG
        skip_count = cycle_length - (cur_block % cycle_length)
        if skip_count != 0:
            self.bump_mocktime(1)
            self.generate(self.nodes[0], skip_count, sync_fun=lambda: self.sync_blocks(nodes))
        self.log.info('Moved from block %d to %d' % (cur_block, self.nodes[0].getblockcount()))


if __name__ == '__main__':
    LLMQQuorumRotationTest().main()
