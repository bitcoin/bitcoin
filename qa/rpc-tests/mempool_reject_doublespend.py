#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test rejection of double-spends in mempool
# Brought on by bugs in the original #5347
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *
import os
import shutil

# Create one-input, one-output, no-fee transaction:
class MempoolSpendCoinbaseTest(BitcoinTestFramework):

    def setup_network(self):
        # Just need one node for this test
        args = ["-checkmempool", "-debug=mempool", "-relaypriority=0"]
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        self.is_network_split = False

    def create_tx(self, from_txid, to_address, amount):
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        signresult = self.nodes[0].signrawtransaction(rawtx)
        assert_equal(signresult["complete"], True)
        return signresult["hex"]

    def run_test(self):
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)

        coinbase_txid = self.nodes[0].getblock(self.nodes[0].getblockhash(101))['tx'][0]
        tx1 = self.create_tx(coinbase_txid, self.nodes[0].getnewaddress(), 50)
        tx2 = self.create_tx(coinbase_txid, self.nodes[0].getnewaddress(), 50)

        spend_id = self.nodes[0].sendrawtransaction(tx1)
        assert_raises(JSONRPCException, self.nodes[0].sendrawtransaction, tx2)

        # mempool should have just spend_101:
        assert_equal(self.nodes[0].getrawmempool(), [ spend_id ])

        tx3 = self.create_tx(spend_id, self.nodes[0].getnewaddress(), 50)
        tx4 = self.create_tx(spend_id, self.nodes[0].getnewaddress(), 50)

        spend2_id = self.nodes[0].sendrawtransaction(tx3)
        assert_raises(JSONRPCException, self.nodes[0].sendrawtransaction, tx4)

        assert_equal(sorted(self.nodes[0].getrawmempool()), sorted([ spend_id, spend2_id ]))

        # mine a block, spend_101 should get confirmed
        self.nodes[0].setgenerate(True, 1)
        assert_equal(set(self.nodes[0].getrawmempool()), set())

if __name__ == '__main__':
    MempoolSpendCoinbaseTest().main()
