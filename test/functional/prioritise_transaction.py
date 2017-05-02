#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the prioritisetransaction mining RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import COIN, MAX_BLOCK_BASE_SIZE

class PrioritiseTransactionTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-printpriority=1"]]

    def run_test(self):
        self.txouts = gen_return_txouts()
        self.relayfee = self.nodes[0].getnetworkinfo()['relayfee']

        utxo_count = 90
        utxos = create_confirmed_utxos(self.relayfee, self.nodes[0], utxo_count)
        base_fee = self.relayfee*100 # our transactions are smaller than 100kb
        txids = []

        # Create 3 batches of transactions at 3 different fee rate levels
        range_size = utxo_count // 3
        for i in range(3):
            txids.append([])
            start_range = i * range_size
            end_range = start_range + range_size
            txids[i] = create_lots_of_big_transactions(self.nodes[0], self.txouts, utxos[start_range:end_range], end_range - start_range, (i+1)*base_fee)

        # Make sure that the size of each group of transactions exceeds
        # MAX_BLOCK_BASE_SIZE -- otherwise the test needs to be revised to create
        # more transactions.
        mempool = self.nodes[0].getrawmempool(True)
        sizes = [0, 0, 0]
        for i in range(3):
            for j in txids[i]:
                assert(j in mempool)
                sizes[i] += mempool[j]['size']
            assert(sizes[i] > MAX_BLOCK_BASE_SIZE) # Fail => raise utxo_count

        # add a fee delta to something in the cheapest bucket and make sure it gets mined
        # also check that a different entry in the cheapest bucket is NOT mined
        self.nodes[0].prioritisetransaction(txids[0][0], int(3*base_fee*COIN))

        self.nodes[0].generate(1)

        mempool = self.nodes[0].getrawmempool()
        self.log.info("Assert that prioritised transaction was mined")
        assert(txids[0][0] not in mempool)
        assert(txids[0][1] in mempool)

        high_fee_tx = None
        for x in txids[2]:
            if x not in mempool:
                high_fee_tx = x

        # Something high-fee should have been mined!
        assert(high_fee_tx != None)

        # Add a prioritisation before a tx is in the mempool (de-prioritising a
        # high-fee transaction so that it's now low fee).
        self.nodes[0].prioritisetransaction(high_fee_tx, -int(2*base_fee*COIN))

        # Add everything back to mempool
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # Check to make sure our high fee rate tx is back in the mempool
        mempool = self.nodes[0].getrawmempool()
        assert(high_fee_tx in mempool)

        # Now verify the modified-high feerate transaction isn't mined before
        # the other high fee transactions. Keep mining until our mempool has
        # decreased by all the high fee size that we calculated above.
        while (self.nodes[0].getmempoolinfo()['bytes'] > sizes[0] + sizes[1]):
            self.nodes[0].generate(1)

        # High fee transaction should not have been mined, but other high fee rate
        # transactions should have been.
        mempool = self.nodes[0].getrawmempool()
        self.log.info("Assert that de-prioritised transaction is still in mempool")
        assert(high_fee_tx in mempool)
        for x in txids[2]:
            if (x != high_fee_tx):
                assert(x not in mempool)

        # Create a free transaction.  Should be rejected.
        utxo_list = self.nodes[0].listunspent()
        assert(len(utxo_list) > 0)
        utxo = utxo_list[0]

        inputs = []
        outputs = {}
        inputs.append({"txid" : utxo["txid"], "vout" : utxo["vout"]})
        outputs[self.nodes[0].getnewaddress()] = utxo["amount"]
        raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
        tx_hex = self.nodes[0].signrawtransaction(raw_tx)["hex"]
        tx_id = self.nodes[0].decoderawtransaction(tx_hex)["txid"]

        # This will raise an exception due to min relay fee not being met
        assert_raises_jsonrpc(-26, "66: min relay fee not met", self.nodes[0].sendrawtransaction, tx_hex)
        assert(tx_id not in self.nodes[0].getrawmempool())

        # This is a less than 1000-byte transaction, so just set the fee
        # to be the minimum for a 1000 byte transaction and check that it is
        # accepted.
        self.nodes[0].prioritisetransaction(tx_id, int(self.relayfee*COIN))

        self.log.info("Assert that prioritised free transaction is accepted to mempool")
        assert_equal(self.nodes[0].sendrawtransaction(tx_hex), tx_id)
        assert(tx_id in self.nodes[0].getrawmempool())

if __name__ == '__main__':
    PrioritiseTransactionTest().main()
