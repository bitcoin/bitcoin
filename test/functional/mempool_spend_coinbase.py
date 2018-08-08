#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test spending coinbase transactions.

The coinbase transaction in block N can appear in block
N+100... so is valid in the mempool when the best block
height is N+99.
This test makes sure coinbase spends that will be mature
in the next block are accepted into the memory pool,
but less mature coinbase spends are NOT.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class MempoolSpendCoinbaseTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)
        node0_address = self.nodes[0].getnewaddress()

        # Coinbase at height chain_height-100+1 ok in mempool, should
        # get mined. Coinbase at height chain_height-100+2 is
        # is too immature to spend.
        b = [ self.nodes[0].getblockhash(n) for n in range(101, 103) ]
        coinbase_txids = [ self.nodes[0].getblock(h)['tx'][0] for h in b ]
        spends_raw = [ create_tx(self.nodes[0], txid, node0_address, 49.99) for txid in coinbase_txids ]

        spend_101_id = self.nodes[0].sendrawtransaction(spends_raw[0])

        # coinbase at height 102 should be too immature to spend
        assert_raises_rpc_error(-26,"bad-txns-premature-spend-of-coinbase", self.nodes[0].sendrawtransaction, spends_raw[1])

        # mempool should have just spend_101:
        assert_equal(self.nodes[0].getrawmempool(), [ spend_101_id ])

        # mine a block, spend_101 should get confirmed
        self.nodes[0].generate(1)
        assert_equal(set(self.nodes[0].getrawmempool()), set())

        # ... and now height 102 can be spent:
        spend_102_id = self.nodes[0].sendrawtransaction(spends_raw[1])
        assert_equal(self.nodes[0].getrawmempool(), [ spend_102_id ])

if __name__ == '__main__':
    MempoolSpendCoinbaseTest().main()
