#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test PrioritiseTransaction code
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

COIN = 100000000

class PrioritiseTransactionTest(BitcoinTestFramework):

    def __init__(self):
        self.txouts = gen_return_txouts()

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-printpriority=1"]))
        self.relayfee = self.nodes[0].getnetworkinfo()['relayfee']

    def run_test(self):
        utxos = create_confirmed_utxos(self.relayfee, self.nodes[0], 90)
        base_fee = self.relayfee*100 # our transactions are smaller than 100kb
        txids = []

        # Create 3 batches of transactions at 3 different fee rate levels
        for i in xrange(3):
            txids.append([])
            txids[i] = create_lots_of_big_transactions(self.nodes[0], self.txouts, utxos[30*i:30*i+30], (i+1)*base_fee)

        # add a fee delta to something in the cheapest bucket and make sure it gets mined
        # also check that a different entry in the cheapest bucket is NOT mined (lower
        # the priority to ensure its not mined due to priority)
        self.nodes[0].prioritisetransaction(txids[0][0], 0, int(3*base_fee*COIN))
        self.nodes[0].prioritisetransaction(txids[0][1], -1e15, 0)

        self.nodes[0].generate(1)

        mempool = self.nodes[0].getrawmempool()
        print "Assert that prioritised transasction was mined"
        assert(txids[0][0] not in mempool)
        assert(txids[0][1] in mempool)

        high_fee_tx = None
        for x in txids[2]:
            if x not in mempool:
                high_fee_tx = x

        # Something high-fee should have been mined!
        assert(high_fee_tx != None)

        # Add a prioritisation before a tx is in the mempool (de-prioritising a
        # high-fee transaction).
        self.nodes[0].prioritisetransaction(high_fee_tx, -1e15, -int(2*base_fee*COIN))

        # Add everything back to mempool
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # Check to make sure our high fee rate tx is back in the mempool
        mempool = self.nodes[0].getrawmempool()
        assert(high_fee_tx in mempool)

        # Now verify the high feerate transaction isn't mined.
        self.nodes[0].generate(5)

        # High fee transaction should not have been mined, but other high fee rate
        # transactions should have been.
        mempool = self.nodes[0].getrawmempool()
        print "Assert that de-prioritised transaction is still in mempool"
        assert(high_fee_tx in mempool)
        for x in txids[2]:
            if (x != high_fee_tx):
                assert(x not in mempool)

if __name__ == '__main__':
    PrioritiseTransactionTest().main()
