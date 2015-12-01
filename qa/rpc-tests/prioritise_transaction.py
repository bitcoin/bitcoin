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
        # Some pre-processing to create a bunch of OP_RETURN txouts to insert into transactions we create
        # So we have big transactions (and therefore can't fit very many into each block)
        # create one script_pubkey
        script_pubkey = "6a4d0200" #OP_RETURN OP_PUSH2 512 bytes
        for i in xrange (512):
            script_pubkey = script_pubkey + "01"
        # concatenate 128 txouts of above script_pubkey which we'll insert before the txout for change
        self.txouts = "81"
        for k in xrange(128):
            # add txout value
            self.txouts = self.txouts + "0000000000000000"
            # add length of script_pubkey
            self.txouts = self.txouts + "fd0402"
            # add script_pubkey
            self.txouts = self.txouts + script_pubkey

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-printpriority=1"]))
        self.relayfee = self.nodes[0].getnetworkinfo()['relayfee']

    def create_confirmed_utxos(self, count):
        self.nodes[0].generate(int(0.5*count)+101)
        utxos = self.nodes[0].listunspent()
        iterations = count - len(utxos)
        addr1 = self.nodes[0].getnewaddress()
        addr2 = self.nodes[0].getnewaddress()
        if iterations <= 0:
            return utxos
        for i in xrange(iterations):
            t = utxos.pop()
            fee = self.relayfee
            inputs = []
            inputs.append({ "txid" : t["txid"], "vout" : t["vout"]})
            outputs = {}
            send_value = t['amount'] - fee
            outputs[addr1] = satoshi_round(send_value/2)
            outputs[addr2] = satoshi_round(send_value/2)
            raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
            signed_tx = self.nodes[0].signrawtransaction(raw_tx)["hex"]
            txid = self.nodes[0].sendrawtransaction(signed_tx)

        while (self.nodes[0].getmempoolinfo()['size'] > 0):
            self.nodes[0].generate(1)

        utxos = self.nodes[0].listunspent()
        assert(len(utxos) >= count)
        return utxos

    def create_lots_of_big_transactions(self, utxos, fee):
        addr = self.nodes[0].getnewaddress()
        txids = []
        for i in xrange(len(utxos)):
            t = utxos.pop()
            inputs = []
            inputs.append({ "txid" : t["txid"], "vout" : t["vout"]})
            outputs = {}
            send_value = t['amount'] - fee
            outputs[addr] = satoshi_round(send_value)
            rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
            newtx = rawtx[0:92]
            newtx = newtx + self.txouts
            newtx = newtx + rawtx[94:]
            signresult = self.nodes[0].signrawtransaction(newtx, None, None, "NONE")
            txid = self.nodes[0].sendrawtransaction(signresult["hex"], True)
            txids.append(txid)
        return txids

    def run_test(self):
        utxos = self.create_confirmed_utxos(90)
        base_fee = self.relayfee*100 # our transactions are smaller than 100kb
        txids = []

        # Create 3 batches of transactions at 3 different fee rate levels
        for i in xrange(3):
            txids.append([])
            txids[i] = self.create_lots_of_big_transactions(utxos[30*i:30*i+30], (i+1)*base_fee)

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
