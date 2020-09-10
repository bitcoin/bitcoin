#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.blocktools import create_block, create_coinbase, get_masternode_payment
from test_framework.mininode import *
from test_framework.script import CScript
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, get_bip9_status, hex_str_to_bytes

'''
feature_block_reward_reallocation.py

Checks block reward reallocation correctness

'''

class BlockRewardReallocationTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)
        self.set_dash_dip8_activation(450)

    # 536870912 == 0x20000000, i.e. not signalling for anything
    def create_test_block(self, version=536870912):
        self.bump_mocktime(150)
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
            self.nodes[0].p2p.send_blocks_and_test([test_block], self.nodes[0], timeout=5)
        # generate at most 10 signaling blocks at a time
        for i in range((num_blocks - 1) // 10):
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
            self.sync_blocks()
        self.nodes[0].generate((num_blocks - 1) % 10)
        self.sync_blocks()
        assert_equal(get_bip9_status(self.nodes[0], 'realloc')['status'], 'started')
        bestblockhash = self.nodes[0].generate(1)[0]
        self.sync_blocks()
        self.nodes[0].getblock(bestblockhash, 1)
        if expected_lockin:
            assert_equal(get_bip9_status(self.nodes[0], 'realloc')['status'], 'locked_in')
        else:
            assert_equal(get_bip9_status(self.nodes[0], 'realloc')['status'], 'started')

    def run_test(self):
        self.log.info("Wait for DIP3 to activate")
        while get_bip9_status(self.nodes[0], 'dip0003')['status'] != 'active':
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
        self.sync_blocks()

        self.nodes[0].add_p2p_connection(P2PDataStore())
        network_thread_start()
        self.nodes[0].p2p.wait_for_verack()

        self.log.info("Mine all but one remaining block in the window")
        bi = self.nodes[0].getblockchaininfo()
        for i in range(498 - bi['blocks']):
            self.bump_mocktime(1)
            self.nodes[0].generate(1)
        self.sync_blocks()

        self.log.info("Initial state is DEFINED")
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 498)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'defined')

        self.log.info("Advance from DEFINED to STARTED at height = 499")
        self.nodes[0].generate(1)
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 499)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'started')
        assert_equal(bi['bip9_softforks']['realloc']['statistics']['threshold'], 400)

        self.signal(399, False) # 1 block short
        self.signal(400, True) # just enough to lock in

        self.log.info("Still LOCKED_IN at height = 1498")
        for i in range(49):
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
            self.sync_blocks()
        self.nodes[0].generate(9)
        self.sync_blocks()
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 1998)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'locked_in')

        self.log.info("Advance from LOCKED_IN to ACTIVE at height = 1999")
        self.nodes[0].generate(1) # activation
        bi = self.nodes[0].getblockchaininfo()
        assert_equal(bi['blocks'], 1999)
        assert_equal(bi['bip9_softforks']['realloc']['status'], 'active')
        assert_equal(bi['bip9_softforks']['realloc']['since'], 2000)

        self.log.info("Reward split should stay ~50/50 before the first superblock after activation")
        # This applies even if reallocation was activated right at superblock height like it does here
        bt = self.nodes[0].getblocktemplate()
        assert_equal(bt['height'], 2000)
        assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2000))
        self.nodes[0].generate(9)
        self.sync_blocks()
        bt = self.nodes[0].getblocktemplate()
        assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2000))
        assert_equal(bt['coinbasevalue'], 17171634268)
        assert_equal(bt['masternode'][0]['amount'], 8585817128) # 0.4999999997

        self.log.info("Reallocation should kick-in with the superblock mined at height = 2010")
        for period in range(19): # there will be 19 adjustments, 3 superblocks long each
            for i in range(3):
                self.bump_mocktime(10)
                self.nodes[0].generate(10)
                self.sync_blocks()
                bt = self.nodes[0].getblocktemplate()
                assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2000))

        self.log.info("Reward split should reach ~60/40 after reallocation is done")
        assert_equal(bt['coinbasevalue'], 12766530779)
        assert_equal(bt['masternode'][0]['amount'], 7659918467) # 0.6

        self.log.info("Reward split should stay ~60/40 after reallocation is done")
        for period in range(10): # check 10 next superblocks
            self.bump_mocktime(10)
            self.nodes[0].generate(10)
            self.sync_blocks()
            bt = self.nodes[0].getblocktemplate()
            assert_equal(bt['masternode'][0]['amount'], get_masternode_payment(bt['height'], bt['coinbasevalue'], 2000))
        assert_equal(bt['coinbasevalue'], 12766530779)
        assert_equal(bt['masternode'][0]['amount'], 7659918467) # 0.6

if __name__ == '__main__':
    BlockRewardReallocationTest().main()
