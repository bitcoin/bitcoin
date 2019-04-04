#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from collections import namedtuple

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
dip4-coinbasemerkleroots.py

Checks DIP4 merkle roots in coinbases

'''

class TestNode(SingleNodeConnCB):
    def __init__(self):
        SingleNodeConnCB.__init__(self)
        self.last_mnlistdiff = None

    def on_mnlistdiff(self, conn, message):
        self.last_mnlistdiff = message

    def wait_for_mnlistdiff(self, timeout=30):
        self.last_mnlistdiff = None
        def received_mnlistdiff():
            return self.last_mnlistdiff is not None
        return wait_until(received_mnlistdiff, timeout=timeout)

    def getmnlistdiff(self, baseBlockHash, blockHash):
        msg = msg_getmnlistd(baseBlockHash, blockHash)
        self.send_message(msg)
        self.wait_for_mnlistdiff()
        return self.last_mnlistdiff


class LLMQCoinbaseCommitmentsTest(DashTestFramework):
    def __init__(self):
        super().__init__(6, 5, [], fast_dip3_enforcement=True)

    def run_test(self):
        self.test_node = TestNode()
        self.test_node.add_connection(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.test_node))
        NetworkThread().start()  # Start up network handling in another thread
        self.test_node.wait_for_verack()

        self.confirm_mns()

        null_hash = format(0, "064x")

        # Check if a diff with the genesis block as base returns all MNs
        expectedUpdated = [mn.proTxHash for mn in self.mninfo]
        mnList = self.test_getmnlistdiff(null_hash, self.nodes[0].getbestblockhash(), {}, [], expectedUpdated)
        expectedUpdated2 = expectedUpdated + []

        # Register one more MN, but don't start it (that would fail as DashTestFramework doesn't support this atm)
        baseBlockHash = self.nodes[0].getbestblockhash()
        self.prepare_masternode(self.mn_count)
        new_mn = self.mninfo[self.mn_count]

        # Now test if that MN appears in a diff when the base block is the one just before MN registration
        expectedDeleted = []
        expectedUpdated = [new_mn.proTxHash]
        mnList = self.test_getmnlistdiff(baseBlockHash, self.nodes[0].getbestblockhash(), mnList, expectedDeleted, expectedUpdated)
        assert(mnList[new_mn.proTxHash].confirmedHash == 0)
        # Now let the MN get enough confirmations and verify that the MNLISTDIFF now has confirmedHash != 0
        self.confirm_mns()
        mnList = self.test_getmnlistdiff(baseBlockHash, self.nodes[0].getbestblockhash(), mnList, expectedDeleted, expectedUpdated)
        assert(mnList[new_mn.proTxHash].confirmedHash != 0)

        # Spend the collateral of the previously added MN and test if it appears in "deletedMNs"
        expectedDeleted = [new_mn.proTxHash]
        expectedUpdated = []
        baseBlockHash2 = self.nodes[0].getbestblockhash()
        self.remove_mastermode(self.mn_count)
        mnList = self.test_getmnlistdiff(baseBlockHash2, self.nodes[0].getbestblockhash(), mnList, expectedDeleted, expectedUpdated)

        # When comparing genesis and best block, we shouldn't see the previously added and then deleted MN
        mnList = self.test_getmnlistdiff(null_hash, self.nodes[0].getbestblockhash(), {}, [], expectedUpdated2)

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
            hashes.append(hash256(mn.serialize()))
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

        return d

    def confirm_mns(self):
        while True:
            diff = self.nodes[0].protx("diff", 1, self.nodes[0].getblockcount())
            found_unconfirmed = False
            for mn in diff["mnList"]:
                if int(mn["confirmedHash"], 16) == 0:
                    found_unconfirmed = True
                    break
            if not found_unconfirmed:
                break
            self.nodes[0].generate(1)
        sync_blocks(self.nodes)

if __name__ == '__main__':
    LLMQCoinbaseCommitmentsTest().main()
