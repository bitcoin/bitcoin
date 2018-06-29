#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal

'''
feature_dip4_coinbasemerkleroots.py

Checks DIP4 merkle roots in coinbases

'''

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


class LLMQCoinbaseCommitmentsTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)

    def run_test(self):
        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        self.nodes[0].p2p.wait_for_verack()

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
        self.remove_masternode(self.mn_count)
        mnList = self.test_getmnlistdiff(baseBlockHash2, self.nodes[0].getbestblockhash(), mnList, expectedDeleted, expectedUpdated)

        # When comparing genesis and best block, we shouldn't see the previously added and then deleted MN
        mnList = self.test_getmnlistdiff(null_hash, self.nodes[0].getbestblockhash(), {}, [], expectedUpdated2)

        #############################
        # Now start testing quorum commitment merkle roots

        self.nodes[0].generate(1)
        oldhash = self.nodes[0].getbestblockhash()
        # Have to disable ChainLocks here because they won't let you to invalidate already locked blocks
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        # Test DIP8 activation once with a pre-existing quorum and once without (we don't know in which order it will activate on mainnet)
        self.test_dip8_quorum_merkle_root_activation(True)
        for n in self.nodes:
            n.invalidateblock(oldhash)
        self.sync_all()
        first_quorum = self.test_dip8_quorum_merkle_root_activation(False, True)

        # Re-enable ChainLocks again
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # Verify that the first quorum appears in MNLISTDIFF
        expectedDeleted = []
        expectedNew = [QuorumId(100, int(first_quorum, 16))]
        quorumList = self.test_getmnlistdiff_quorums(null_hash, self.nodes[0].getbestblockhash(), {}, expectedDeleted, expectedNew)
        baseBlockHash = self.nodes[0].getbestblockhash()

        second_quorum = self.mine_quorum()

        # Verify that the second quorum appears in MNLISTDIFF
        expectedDeleted = []
        expectedNew = [QuorumId(100, int(second_quorum, 16))]
        quorums_before_third = self.test_getmnlistdiff_quorums(baseBlockHash, self.nodes[0].getbestblockhash(), quorumList, expectedDeleted, expectedNew)
        block_before_third = self.nodes[0].getbestblockhash()

        third_quorum = self.mine_quorum()

        # Verify that the first quorum is deleted and the third quorum is added in MNLISTDIFF (the first got inactive)
        expectedDeleted = [QuorumId(100, int(first_quorum, 16))]
        expectedNew = [QuorumId(100, int(third_quorum, 16))]
        self.test_getmnlistdiff_quorums(block_before_third, self.nodes[0].getbestblockhash(), quorums_before_third, expectedDeleted, expectedNew)

        # Verify that the diff between genesis and best block is the current active set (second and third quorum)
        expectedDeleted = []
        expectedNew = [QuorumId(100, int(second_quorum, 16)), QuorumId(100, int(third_quorum, 16))]
        self.test_getmnlistdiff_quorums(null_hash, self.nodes[0].getbestblockhash(), {}, expectedDeleted, expectedNew)

        # Now verify that diffs are correct around the block that mined the third quorum.
        # This tests the logic in CalcCbTxMerkleRootQuorums, which has to manually add the commitment from the current
        # block
        mined_in_block = self.nodes[0].quorum("info", 100, third_quorum)["minedBlock"]
        prev_block = self.nodes[0].getblock(mined_in_block)["previousblockhash"]
        prev_block2 = self.nodes[0].getblock(prev_block)["previousblockhash"]
        next_block = self.nodes[0].getblock(mined_in_block)["nextblockhash"]
        next_block2 = self.nodes[0].getblock(mined_in_block)["nextblockhash"]
        # The 2 block before the quorum was mined should both give an empty diff
        expectedDeleted = []
        expectedNew = []
        self.test_getmnlistdiff_quorums(block_before_third, prev_block2, quorums_before_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(block_before_third, prev_block, quorums_before_third, expectedDeleted, expectedNew)
        # The block in which the quorum was mined and the 2 after that should all give the same diff
        expectedDeleted = [QuorumId(100, int(first_quorum, 16))]
        expectedNew = [QuorumId(100, int(third_quorum, 16))]
        quorums_with_third = self.test_getmnlistdiff_quorums(block_before_third, mined_in_block, quorums_before_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(block_before_third, next_block, quorums_before_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(block_before_third, next_block2, quorums_before_third, expectedDeleted, expectedNew)
        # A diff between the two block that happened after the quorum was mined should give an empty diff
        expectedDeleted = []
        expectedNew = []
        self.test_getmnlistdiff_quorums(mined_in_block, next_block, quorums_with_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(mined_in_block, next_block2, quorums_with_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(next_block, next_block2, quorums_with_third, expectedDeleted, expectedNew)

        # Using the same block for baseBlockHash and blockHash should give empty diffs
        self.test_getmnlistdiff_quorums(prev_block, prev_block, quorums_before_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(prev_block2, prev_block2, quorums_before_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(mined_in_block, mined_in_block, quorums_with_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(next_block, next_block, quorums_with_third, expectedDeleted, expectedNew)
        self.test_getmnlistdiff_quorums(next_block2, next_block2, quorums_with_third, expectedDeleted, expectedNew)


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

    def test_dip8_quorum_merkle_root_activation(self, with_initial_quorum, slow_mode=False):
        if with_initial_quorum:
            self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
            self.wait_for_sporks_same()

            # Mine one quorum before dip8 is activated
            self.mine_quorum()

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 4070908800)
        self.wait_for_sporks_same()

        cbtx = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]
        assert(cbtx["cbTx"]["version"] == 1)

        self.activate_dip8(slow_mode)

        # Assert that merkleRootQuorums is present and 0 (we have no quorums yet)
        cbtx = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]
        assert_equal(cbtx["cbTx"]["version"], 2)
        assert("merkleRootQuorums" in cbtx["cbTx"])
        merkleRootQuorums = int(cbtx["cbTx"]["merkleRootQuorums"], 16)

        if with_initial_quorum:
            assert(merkleRootQuorums != 0)
        else:
            assert_equal(merkleRootQuorums, 0)

        self.bump_mocktime(1)
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # Mine quorum and verify that merkleRootQuorums has changed
        quorum = self.mine_quorum()
        cbtx = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)["tx"][0]
        assert(int(cbtx["cbTx"]["merkleRootQuorums"], 16) != merkleRootQuorums)

        return quorum

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
        self.sync_blocks()

if __name__ == '__main__':
    LLMQCoinbaseCommitmentsTest().main()
