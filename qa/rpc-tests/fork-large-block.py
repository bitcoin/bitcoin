#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import NetworkThread, CTransaction, FromHex
from test_framework.blocktools import create_coinbase, create_block, create_transaction
from test_framework.script import *
import time

'''
This test is meant to exercise forced large block after BIP141 is activated
regtest lock-in with 108/144 for BIP141 and 29/48 for BIP91
mine 143 blocks to transition from DEFINED to STARTED
mine 28 blocks signalling readiness and 20 not in order to fail to change state this period for BIP91
mine 29 blocks signalling readiness and 19 blocks not signalling readiness for BIP91 (STARTED->LOCKED_IN)
bit 1 is optional for the following 48 blocks when BIP91 is LOCKED_IN (LOCKED_IN->ACTIVE)
bit 1 is mandatory for the following 144 blocks until BIP141 is locked_in
bit 1 is optional after BIP141 is locked_in
mine 1007 blocks accept all blocks size next block required to be greater than 1 Mb
'''

class ForkLargeBlockTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-whitelist=127.0.0.1"])) # connect to a dummy node to allow getblocktemplate
        connect_nodes(self.nodes[0], 1)

    def run_test(self):
        NetworkThread().start() # Start up network handling in another thread
        self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.height = 1  # height of the next block to build
        self.last_block_time = int(time.time())

        assert_equal(self.get_bip9_status('segwit2x')['status'], 'defined')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 0)
        assert_equal(self.get_bip9_status('segwit')['status'], 'defined')
        assert_equal(self.get_bip9_status('segwit')['since'], 0)


        # Test 1
        # Advance from DEFINED to STARTED
        self.generate_blocks(1,4)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x20000000)

        self.generate_blocks(142, 4)
        assert_equal(self.get_bip9_status('segwit2x')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 48)
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit')['since'], 144)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000012) # 0x10000001 are TESTDUMMY and CSV

        # Test 2
        # Fail to achieve LOCKED_IN 28 out of 48 signal bit 4
        self.generate_blocks(20, 0x20000010) # signalling bit 4
        self.generate_blocks(8, 0x20000012) # signalling bit 1 and 4
        self.generate_blocks(10, 0x20000002) # signalling bit 1
        self.generate_blocks(10, 4) # not signalling
        assert_equal(self.get_bip9_status('segwit2x')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 48)
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit')['since'], 144)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000012)


        # Test 3
        # 28 out of 48 signal bit 4 to achieve LOCKED_IN
        self.generate_blocks(20, 0x20000010) # signalling bit 4
        self.generate_blocks(9, 0x20000012) # signalling bit 1 and 4
        self.generate_blocks(10, 0x20000002) # signalling bit 1
        self.generate_blocks(9, 4) # not signalling
        assert_equal(self.get_bip9_status('segwit2x')['status'], 'locked_in')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 240)
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit')['since'], 144)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000012)

        # Test 4
        # No restriction when bit 4 is LOCKED_IN
        self.generate_blocks(5, 4)
        self.generate_blocks(5, 0x20000000)
        self.generate_blocks(5, 0x20000010)
        self.generate_blocks(5, 0x40000002)
        self.generate_blocks(5, 0x60000002)
        self.generate_blocks(5, 0x12)
        self.generate_blocks(5, 0x20000002)
        self.generate_blocks(5, 0x20000012)
        self.generate_blocks(8, 0x20000102)
        assert_equal(self.get_bip9_status('segwit2x')['status'], 'active')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 288)
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit')['since'], 144)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000002)

        # Test 5
        # bit 1 signalling becomes mandatory after bit 4 is ACTIVE
        self.generate_blocks(1, 4, 'bad-no-segwit')
        self.generate_blocks(1, 0x20000000, 'bad-no-segwit')
        self.generate_blocks(1, 0x20000010, 'bad-no-segwit')
        self.generate_blocks(1, 0x40000002, 'bad-no-segwit')
        self.generate_blocks(1, 0x60000002, 'bad-no-segwit')
        self.generate_blocks(1, 0x12, 'bad-no-segwit')
        self.generate_blocks(35, 0x20000002)
        self.generate_blocks(35, 0x20000012)
        self.generate_blocks(73, 0x20000102)

        assert_equal(self.get_bip9_status('segwit2x')['status'], 'active')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 288)
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')
        assert_equal(self.get_bip9_status('segwit')['since'], 144)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000002)

        self.generate_blocks(1, 4, 'bad-no-segwit')
        self.generate_blocks(1, 0x20000000, 'bad-no-segwit')
        self.generate_blocks(1, 0x20000010, 'bad-no-segwit')
        self.generate_blocks(1, 0x40000002, 'bad-no-segwit')
        self.generate_blocks(1, 0x60000002, 'bad-no-segwit')
        self.generate_blocks(1, 0x12, 'bad-no-segwit')
        self.generate_blocks(1, 0x20000002)

        # Test 6
        # bit 1 signalling becomes optional after bit 1 locked_in

        assert_equal(self.get_bip9_status('segwit2x')['status'], 'active')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 288)
        assert_equal(self.get_bip9_status('segwit')['status'], 'locked_in')
        assert_equal(self.get_bip9_status('segwit')['since'], 432)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000002)

        self.generate_blocks(20, 0x20000002)
        self.generate_blocks(20, 0x20000012)
        self.generate_blocks(20, 0x20000102)
        self.generate_blocks(20, 0x20000000)
        self.generate_blocks(20, 0x20000010)
        self.generate_blocks(20, 0x40000002)
        self.generate_blocks(23, 0x60000002)


        assert_equal(self.get_bip9_status('segwit2x')['status'], 'active')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 288)
        assert_equal(self.get_bip9_status('segwit')['status'], 'locked_in')
        assert_equal(self.get_bip9_status('segwit')['since'], 432)

        self.generate_blocks(1, 4)


        assert_equal(self.get_bip9_status('segwit2x')['status'], 'active')
        assert_equal(self.get_bip9_status('segwit2x')['since'], 288)
        assert_equal(self.get_bip9_status('segwit')['status'], 'active')
        assert_equal(self.get_bip9_status('segwit')['since'], 576)
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['version'], 0x10000001|0x20000000)

        self.generate_blocks(1, 0x20000002)
        self.generate_blocks(1, 0x20000012)
        self.generate_blocks(1, 0x20000102)
        self.generate_blocks(1, 0x20000000)
        self.generate_blocks(1, 0x20000010)
        self.generate_blocks(1, 0x40000002)
        self.generate_blocks(1, 0x60000002)
        self.generate_blocks(1, 4)

        # Test 7
        # Test hard fork at block 1583
        assert_equal(self.height, 584)

        b = [self.nodes[0].getblockhash(n) for n in range(1, 10)]
        txids = [self.nodes[0].getblock(h)['tx'][0] for h in b]
        spend_tx = [FromHex(CTransaction(), self.nodes[0].getrawtransaction(txid)) for txid in txids]
        for tx in spend_tx:
            tx.rehash()
        large_tx = [self.create_tx(t, 0, 1, length=500000) for t in spend_tx]

        self.generate_blocks(998, 4)

        self.generate_blocks(1, 4, "bad-blk-length", txs=[large_tx[0], large_tx[1]]) # block too large
        self.generate_blocks(1, 4, txs=[large_tx[0]]) # large txs is ok

        assert_equal(self.height, 1583)

        self.generate_blocks(1, 4, "bad-blk-length-toosmall") # block too small

        self.generate_blocks(1, 4, txs=[large_tx[1], large_tx[2], large_tx[3]]) # mandatory large block

        assert_equal(self.height, 1584)

        # large blocks are not required
        for x in range(0, 5):
            self.generate_blocks(1000, 4) # small blocks are ok now

        assert_equal(self.height, 6584)

        self.generate_blocks(1, 4, txs=[large_tx[4], large_tx[5], large_tx[6]]) # large block ok


    def generate_blocks(self, number, version, error = None, txs = []):
        for i in range(number):
            block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
            block.nVersion = version
            if len(txs) > 0:
                block.vtx.extend(txs)
                block.hashMerkleRoot = block.calc_merkle_root()
            block.rehash()
            block.solve()
            assert_equal(self.nodes[0].submitblock(bytes_to_hex_str(block.serialize())), error)
            if (error == None):
                self.last_block_time += 1
                self.tip = block.sha256
                self.height += 1

    def get_bip9_status(self, key):
        info = self.nodes[0].getblockchaininfo()
        return info['bip9_softforks'][key]

    def create_tx(self, spend_tx, n, value, script=CScript([OP_TRUE]), length=0):
        tx = create_transaction(spend_tx, n, b"", value, script)
        if length > 0:
            script_length = length
            script_output = CScript([b'\x00' * script_length])
            tx.vout.append(CTxOut(0, script_output))
            tx.rehash()
        return tx


if __name__ == '__main__':
    ForkLargeBlockTest().main()
