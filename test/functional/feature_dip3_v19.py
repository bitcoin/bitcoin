#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Dash Core developers
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
    assert_equal,
    p2p_port
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
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)
        for i in range(0, len(self.nodes)):
            self.extra_args[i].append("-dip19params=200")

    def run_test(self):
        # Connect all nodes to node1 so that we always have the whole network connected
        # Otherwise only masternode connections will be established between nodes, which won't propagate TXs/blocks
        # Usually node0 is the one that does this, but in this test we isolate it multiple times

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        null_hash = format(0, "064x")

        for i in range(len(self.nodes)):
            if i != 0:
                self.connect_nodes(i, 0)


        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
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
        self.mine_quorum()

        for mn in self.mninfo:
            self.log.info(f"Trying to update MN payee")
            self.update_mn_payee(mn, self.nodes[0].getnewaddress())

            self.log.info(f"Trying to update MN service")
            self.test_protx_update_service(mn)

        mn = self.mninfo[-1]
        revoke_protx = mn.proTxHash
        revoke_keyoperator = mn.keyOperator
        self.log.info(f"Trying to revoke proTx:{revoke_protx}")
        self.test_revoke_protx(revoke_protx, revoke_keyoperator)

        self.mine_quorum()

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

    def update_mn_payee(self, mn, payee):
        self.nodes[0].sendtoaddress(mn.collateral_address, 0.001)
        self.nodes[0].protx_update_registrar(mn.proTxHash, '', '', payee, mn.collateral_address)
        self.generate(self.nodes[0], 1)
        info = self.nodes[0].protx_info(mn.proTxHash)
        assert info['state']['payoutAddress'] == payee

    def test_protx_update_service(self, mn):
        self.nodes[0].sendtoaddress(mn.collateral_address, 0.001)
        self.nodes[0].protx_update_service( mn.proTxHash, '127.0.0.2:%d' % p2p_port(mn.nodeIdx), mn.keyOperator, "", mn.collateral_address)
        self.generate(self.nodes[0], 1)
        for node in self.nodes:
            protx_info = node.protx_info( mn.proTxHash)
            assert_equal(protx_info['state']['service'], '127.0.0.2:%d' % p2p_port(mn.nodeIdx))

        # undo
        self.nodes[0].protx_update_service(mn.proTxHash, '127.0.0.1:%d' % p2p_port(mn.nodeIdx), mn.keyOperator, "", mn.collateral_address)
        self.generate(self.nodes[0], 1)

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

        # Verify that the merkle root matches what we locally calculate
        hashes = []
        for mn in sorted(newMNList.values(), key=lambda mn: ser_uint256(mn.proRegTxHash)):
            hashes.append(hash256(mn.serialize()))
        merkleRoot = CBlock.get_merkle_root(hashes)
        assert_equal(merkleRoot, d.merkleRootMNList)

        return newMNList

    def test_getmnlistdiff_base(self, baseBlockHash, blockHash):
        hexstr = self.nodes[0].getblockheader(blockHash, False)
        header = from_hex(CBlockHeader(), hexstr)

        d = self.test_node.getmnlistdiff(int(baseBlockHash, 16), int(blockHash, 16))
        assert_equal(d.baseBlockHash, int(baseBlockHash, 16))
        assert_equal(d.blockHash, int(blockHash, 16))

        # Check that the merkle proof is valid
        proof = CMerkleBlock()
        proof.header = header
        proof.txn = d.merkleProof
        proof = proof.serialize().hex()
        # merkle proof first hash should be coinbase
        assert_equal(self.nodes[0].verifytxoutproof(proof), [format(d.merkleProof.vHash[0], '064x')])

        # Check if P2P messages match with RPCs
        d2 = self.nodes[0].protx_diff(baseBlockHash, blockHash)
        assert_equal(d2["baseBlockHash"], baseBlockHash)
        assert_equal(d2["blockHash"], blockHash)
        assert_equal(d2["cbTxMerkleTree"], d.merkleProof.serialize().hex())
        assert_equal(set([int(e, 16) for e in d2["deletedMNs"]]), set(d.deletedMNs))
        assert_equal(set([int(e["proRegTxHash"], 16) for e in d2["mnList"]]), set([e.proRegTxHash for e in d.mnList]))
        assert_equal(set([QuorumId(e["llmqType"], int(e["quorumHash"], 16)) for e in d2["deletedQuorums"]]), set(d.deletedQuorums))
        assert_equal(set([QuorumId(e["llmqType"], int(e["quorumHash"], 16)) for e in d2["newQuorums"]]), set([QuorumId(e.llmqType, e.quorumHash) for e in d.newQuorums]))

        return d


if __name__ == '__main__':
    DIP3V19Test().main()
