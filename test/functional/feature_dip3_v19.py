#!/usr/bin/env python3
# Copyright (c) 2015-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_dip3_v19.py

Checks DIP3 for v19

'''
from io import BytesIO

from test_framework.p2p import P2PInterface
from test_framework.messages import CBlock, CBlockHeader, CCbTx, CMerkleBlock, from_hex, hash256, msg_getmnlistd, \
    QuorumId, ser_uint256
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal
)


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

    def getmnlistdiff(self, base_block_hash, block_hash):
        msg = msg_getmnlistd(base_block_hash, block_hash)
        self.last_mnlistdiff = None
        self.send_message(msg)
        self.wait_for_mnlistdiff()
        return self.last_mnlistdiff


class DIP3V19Test(DashTestFramework):
    def set_test_params(self):
        self.extra_args = [[
            '-testactivationheight=v19@200',
        ]] * 6
        self.set_dash_test_params(6, 5, evo_count=2, extra_args=self.extra_args)
        self.delay_v20_and_mn_rr(height=260)


    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        null_hash = format(0, "064x")

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        expected_updated = [mn.proTxHash for mn in self.mninfo]
        b_0 = self.nodes[0].getbestblockhash()
        self.test_getmnlistdiff(null_hash, b_0, {}, [], expected_updated)

        mn_list_before = self.nodes[0].masternodelist()
        pubkeyoperator_list_before = set([mn_list_before[e]["pubkeyoperator"] for e in mn_list_before])

        self.mine_quorum(llmq_type_name='llmq_test', llmq_type=100)

        self.activate_by_name('v19', expected_activation_height=200)
        self.log.info("Activated v19 at height:" + str(self.nodes[0].getblockcount()))

        mn_list_after = self.nodes[0].masternodelist()
        pubkeyoperator_list_after = set([mn_list_after[e]["pubkeyoperator"] for e in mn_list_after])

        self.log.info("pubkeyoperator should still be shown using legacy scheme")
        assert_equal(pubkeyoperator_list_before, pubkeyoperator_list_after)

        evo_info_0 = self.dynamically_add_masternode(evo=True, rnd=7)
        assert evo_info_0 is not None

        self.log.info("Checking that protxs with duplicate EvoNodes fields are rejected")
        evo_info_1 = self.dynamically_add_masternode(evo=True, rnd=7, should_be_rejected=True)
        assert evo_info_1 is None
        self.dynamically_evo_update_service(evo_info_0, 8)
        evo_info_2 = self.dynamically_add_masternode(evo=True, rnd=8, should_be_rejected=True)
        assert evo_info_2 is None
        evo_info_3 = self.dynamically_add_masternode(evo=True, rnd=9)
        assert evo_info_3 is not None
        self.dynamically_evo_update_service(evo_info_0, 9, should_be_rejected=True)

        revoke_protx = self.mninfo[-1].proTxHash
        revoke_keyoperator = self.mninfo[-1].keyOperator
        self.log.info(f"Trying to revoke proTx:{revoke_protx}")
        self.test_revoke_protx(evo_info_3.nodeIdx, revoke_protx, revoke_keyoperator)

        self.mine_quorum(llmq_type_name='llmq_test', llmq_type=100)

        self.log.info("Checking that adding more regular MNs after v19 doesn't break DKGs and IS/CLs")

        for i in range(6):
            new_mn = self.dynamically_add_masternode(evo=False, rnd=(10 + i))
            assert new_mn is not None

        self.log.info("Chainlocks in CbTx can be only in Basic scheme. Wait one...")
        self.wait_until(lambda: self.nodes[0].getbestchainlock()['height'] > 200)
        self.log.info(f"block {self.nodes[0].getbestchainlock()['height']} is chainlocked after v19 activation")

        self.activate_by_name('v20', expected_activation_height=260)
        # mine more quorums and make sure everything still works
        prev_quorum = None
        for _ in range(5):
            quorum = self.mine_quorum()
            assert prev_quorum != quorum

        self.wait_for_chainlocked_block_all_nodes(self.nodes[0].getbestblockhash())

    def test_revoke_protx(self, node_idx, revoke_protx, revoke_keyoperator):
        funds_address = self.nodes[0].getnewaddress()
        fund_txid = self.nodes[0].sendtoaddress(funds_address, 1)
        self.bump_mocktime(10 * 60 + 1) # to make tx safe to include in block
        tip = self.generate(self.nodes[0], 1)[0]
        assert_equal(self.nodes[0].getrawtransaction(fund_txid, 1, tip)['confirmations'], 1)

        protx_result = self.nodes[0].protx('revoke', revoke_protx, revoke_keyoperator, 1, funds_address)
        self.bump_mocktime(10 * 60 + 1) # to make tx safe to include in block
        tip = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[0]
        assert_equal(self.nodes[0].getrawtransaction(protx_result, 1, tip)['confirmations'], 1)
        # Revoking a MN results in disconnects. Wait for disconnects to actually happen
        # and then reconnect the corresponding node back to let sync_blocks finish correctly.
        self.wait_until(lambda: self.nodes[node_idx].getconnectioncount() == 0)
        self.connect_nodes(node_idx, 0)
        self.sync_all()
        self.log.info(f"Successfully revoked={revoke_protx}")
        for mn in self.mninfo:
            if mn.proTxHash == revoke_protx:
                self.mninfo.remove(mn)
                return

    def test_getmnlistdiff(self, base_block_hash, block_hash, base_mn_list, expected_deleted, expected_updated):
        d = self.test_getmnlistdiff_base(base_block_hash, block_hash)

        # Assert that the deletedMNs and mnList fields are what we expected
        assert_equal(set(d.deletedMNs), set([int(e, 16) for e in expected_deleted]))
        assert_equal(set([e.proRegTxHash for e in d.mnList]), set(int(e, 16) for e in expected_updated))

        # Build a new list based on the old list and the info from the diff
        new_mn_list = base_mn_list.copy()
        for e in d.deletedMNs:
            new_mn_list.pop(format(e, '064x'))
        for e in d.mnList:
            new_mn_list[format(e.proRegTxHash, '064x')] = e

        cbtx = CCbTx()
        cbtx.deserialize(BytesIO(d.cbTx.vExtraPayload))

        # Verify that the merkle root matches what we locally calculate
        hashes = []
        for mn in sorted(new_mn_list.values(), key=lambda mn: ser_uint256(mn.proRegTxHash)):
            hashes.append(hash256(mn.serialize(with_version = False)))
        merkle_root = CBlock.get_merkle_root(hashes)
        assert_equal(merkle_root, cbtx.merkleRootMNList)

        return new_mn_list

    def test_getmnlistdiff_base(self, base_block_hash, block_hash):
        hexstr = self.nodes[0].getblockheader(block_hash, False)
        header = from_hex(CBlockHeader(), hexstr)

        d = self.test_node.getmnlistdiff(int(base_block_hash, 16), int(block_hash, 16))
        assert_equal(d.baseBlockHash, int(base_block_hash, 16))
        assert_equal(d.blockHash, int(block_hash, 16))

        # Check that the merkle proof is valid
        proof = CMerkleBlock(header, d.merkleProof)
        proof = proof.serialize().hex()
        assert_equal(self.nodes[0].verifytxoutproof(proof), [d.cbTx.hash])

        # Check if P2P messages match with RPCs
        d2 = self.nodes[0].protx("diff", base_block_hash, block_hash)
        assert_equal(d2["baseBlockHash"], base_block_hash)
        assert_equal(d2["blockHash"], block_hash)
        assert_equal(d2["cbTxMerkleTree"], d.merkleProof.serialize().hex())
        assert_equal(d2["cbTx"], d.cbTx.serialize().hex())
        assert_equal(set([int(e, 16) for e in d2["deletedMNs"]]), set(d.deletedMNs))
        assert_equal(set([int(e["proRegTxHash"], 16) for e in d2["mnList"]]), set([e.proRegTxHash for e in d.mnList]))
        assert_equal(set([QuorumId(e["llmqType"], int(e["quorumHash"], 16)) for e in d2["deletedQuorums"]]), set(d.deletedQuorums))
        assert_equal(set([QuorumId(e["llmqType"], int(e["quorumHash"], 16)) for e in d2["newQuorums"]]), set([QuorumId(e.llmqType, e.quorumHash) for e in d.newQuorums]))

        return d


if __name__ == '__main__':
    DIP3V19Test().main()
