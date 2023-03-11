#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_dip3_v19.py

Checks DIP3 for v19

'''
from io import BytesIO

from test_framework.mininode import P2PInterface
from test_framework.messages import CBlock, CBlockHeader, CCbTx, CMerkleBlock, FromHex, hash256, msg_getmnlistd, \
    QuorumId, ser_uint256
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal, wait_until
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
        return wait_until(received_mnlistdiff, timeout=timeout)

    def getmnlistdiff(self, base_block_hash, block_hash):
        msg = msg_getmnlistd(base_block_hash, block_hash)
        self.last_mnlistdiff = None
        self.send_message(msg)
        self.wait_for_mnlistdiff()
        return self.last_mnlistdiff


class DIP3V19Test(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)

    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        null_hash = format(0, "064x")

        for i in range(len(self.nodes)):
            if i != 0:
                self.connect_nodes(i, 0)


        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        expected_updated = [mn.proTxHash for mn in self.mninfo]
        b_0 = self.nodes[0].getbestblockhash()
        self.test_getmnlistdiff(null_hash, b_0, {}, [], expected_updated)

    
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        self.mine_cycle_quorum(llmq_type_name='llmq_test_dip0024', llmq_type=103)

        revoke_protx = self.mninfo[-1].proTxHash
        revoke_keyoperator = self.mninfo[-1].keyOperator
        self.log.info(f"Trying to revoke proTx:{revoke_protx}")
        self.test_revoke_protx(revoke_protx, revoke_keyoperator)

        self.mine_quorum(llmq_type_name='llmq_test', llmq_type=100)

        return

    def test_revoke_protx(self, revoke_protx, revoke_keyoperator):
        funds_address = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(funds_address, 1)
        self.generate(self.nodes[0], 1)
        self.sync_all(self.nodes)

        self.nodes[0].protx_revoke(revoke_protx, revoke_keyoperator, 1, funds_address)
        self.generate(self.nodes[0], 1)
        self.sync_all(self.nodes)
        self.log.info(f"Succesfully revoked={revoke_protx}")
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
            hashes.append(hash256(mn.serialize()))
        merkle_root = CBlock.get_merkle_root(hashes)
        assert_equal(merkle_root, cbtx.merkleRootMNList)

        return new_mn_list

    def test_getmnlistdiff_base(self, base_block_hash, block_hash):
        hexstr = self.nodes[0].getblockheader(block_hash, False)
        header = FromHex(CBlockHeader(), hexstr)

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
