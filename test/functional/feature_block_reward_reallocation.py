#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.blocktools import create_block, create_coinbase, get_masternode_payment
from test_framework.mininode import P2PDataStore
from test_framework.messages import CTxOut, FromHex, CCbTx, CTransaction, ToHex
from test_framework.script import CScript
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, get_bip9_status, hex_str_to_bytes

'''
feature_block_reward_reallocation.py

Checks block reward reallocation correctness

'''


class BlockRewardReallocationTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(2, 1, fast_dip3_enforcement=True)
        self.set_dash_dip8_activation(450)

    # 536870912 == 0x20000000, i.e. not signalling for anything
    def create_test_block(self, version=536870912):
        self.bump_mocktime(5)
        bt = self.nodes[0].getblocktemplate()
        tip = int(bt['previousblockhash'], 16)
        nextheight = bt['height']

        coinbase = create_coinbase(nextheight)
        coinbase.nVersion = 3
        coinbase.nType = 5 # CbTx
        coinbase.vout[0].nValue = bt['coinbasevalue']
        for mn in bt['masternode']:
            coinbase.vout.append(CTxOut(mn['amount'], CScript(hex_str_to_bytes(mn['script']))))
            coinbase.vout[0].nValue -= mn['amount']
        cbtx = FromHex(CCbTx(), bt['coinbase_payload'])
        coinbase.vExtraPayload = cbtx.serialize()
        coinbase.rehash()
        coinbase.calc_sha256()

        block = create_block(tip, coinbase, self.mocktime)
        block.nVersion = version
        # Add quorum commitments from template
        for tx in bt['transactions']:
            tx2 = FromHex(CTransaction(), tx['data'])
            if tx2.nType == 6:
                block.vtx.append(tx2)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        return block

    def signal(self, num_blocks, expected_lockin):
        self.log.info("Signal with %d/500 blocks" % (num_blocks))
        # create and send non-signalling blocks
        for i in range(500 - num_blocks):
            test_block = self.create_test_block()
            self.nodes[0].submitblock(ToHex(test_block))
        # generate at most 10 signaling blocks at a time
        if num_blocks > 0:
            for i in range((num_blocks - 1) // 10):
                self.bump_mocktime(10)
                self.nodes[0].generate(10)
            self.nodes[0].generate((num_blocks - 1) % 10)
            assert_equal(get_bip9_status(self.nodes[0], 'realloc')['status'], 'started')
            self.nodes[0].generate(1)
        if expected_lockin:
            assert_equal(get_bip9_status(self.nodes[0], 'realloc')['status'], 'locked_in')
        else:
            assert_equal(get_bip9_status(self.nodes[0], 'realloc')['status'], 'started')

    def threshold(self, attempt):
        threshold_calc = 400 - attempt * attempt
        if threshold_calc < 300:
            return 300
        return threshold_calc

    def run_test(self):
        self.log.info("Wait for DIP3 to activate")
        while get_bip9_status(self.nodes[0], 'dip0003')['status'] != 'active':
            self.bump_mocktime(10)
            self.nodes[0].generate(10)

        self.nodes[0].add_p2p_connection(P2PDataStore())
        self.nodes[0].p2p.wait_for_verack()

        self.log.info("Mine all but one remaining block in the window")
        bi = self.nodes[0].getblockchaininfo()
        for i in range(498 - bi['blocks']):
            self.bump_mocktime(1)
            self.nodes[0].generate(1)

        self.log.info("Initial state is DEFINED")
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 498)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'defined')

        self.log.info("Advance from DEFINED to STARTED at height = 499")
        self.nodes[0].generate(1)
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 499)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'started')
        assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], self.threshold(0))

        self.signal(399, False)  # 1 block short

        self.log.info("Still STARTED but new threshold should be lower at height = 999")
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 999)
        assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], self.threshold(1))

        self.signal(398, False)  # 1 block short again

        self.log.info("Still STARTED but new threshold should be even lower at height = 1499")
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 1499)
        assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], self.threshold(2))
        pre_locked_in_blockhash = bi['bestblockhash']

        self.signal(396, True)  # just enough to lock in
        self.log.info("Advanced to LOCKED_IN at height = 1999")

        for i in range(49):
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
        self.nodes[0].generate(9)

        self.log.info("Still LOCKED_IN at height = 2498")
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 2498)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'locked_in')

        self.log.info("Advance from LOCKED_IN to ACTIVE at height = 2499")
        self.nodes[0].generate(1)  # activation
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 2499)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'active')
        assert_equal(bi['bip9_softforks']['realloc']['since'], 2500)

        self.log.info("Reward split should stay ~50/50 before the first superblock after activation")
        # This applies even if reallocation was activated right at superblock height like it does here
        bt = self.nodes[0].getblocktemplate()
        assert_equal(bt['height'], 2500)
        assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2500))
        self.nodes[0].generate(9)
        bt = self.nodes[0].getblocktemplate()
        assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2500))
        assert_equal(bt['coinbasevalue'], 13748571607)
        assert_equal(bt['masternode'][0]['amount'], 6874285801) # 0.4999999998

        self.log.info("Reallocation should kick-in with the superblock mined at height = 2010")
        for period in range(19):  # there will be 19 adjustments, 3 superblocks long each
            for i in range(3):
                self.bump_mocktime(10)
                self.nodes[0].generate(10)
                bt = self.nodes[0].getblocktemplate()
                assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2500))

        self.log.info("Reward split should reach ~60/40 after reallocation is done")
        assert_equal(bt['coinbasevalue'], 10221599170)
        assert_equal(bt['masternode'][0]['amount'], 6132959502) # 0.6

        self.log.info("Reward split should stay ~60/40 after reallocation is done")
        for period in range(10):  # check 10 next superblocks
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
            bt = self.nodes[0].getblocktemplate()
            assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2500))
        assert_equal(bt['coinbasevalue'], 9491484944)
        assert_equal(bt['masternode'][0]['amount'], 5694890966) # 0.6

        # make sure all nodes are still synced
        self.sync_all()

        self.log.info("Rollback the chain back to the STARTED state")
        self.mocktime = self.nodes[0].getblock(pre_locked_in_blockhash, 1)['time']
        for node in self.nodes:
            node.invalidateblock(pre_locked_in_blockhash)
        # create and send non-signalling block
        test_block = self.create_test_block()
        self.nodes[0].submitblock(ToHex(test_block))
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 1499)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'started')
        assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], self.threshold(2))

        self.log.info("Check thresholds reach min level and stay there")
        for i in range(8):  # 7 to reach min level and 1 more to check it doesn't go lower than that
            self.signal(0, False)  # no need to signal
            bi = self.nodes[0].getblockchaininfo()
            assert_equal(bi['blocks'], 1999 + i * 500)
            assert_equal(bi['bip9_softforks']['realloc']['status'], 'started')
            assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], self.threshold(i + 3))
        assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], 300)


if __name__ == '__main__':
    BlockRewardReallocationTest().main()
