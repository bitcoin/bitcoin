#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool limiting together/eviction with the wallet."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class MempoolLimitTest(BitcoinTestFramework):

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-maxmempool=5", "-spendzeroconfchange=0"]))
        self.is_network_split = False
        self.sync_all()
        self.relayfee = self.nodes[0].getnetworkinfo()['relayfee']

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

        self.txouts = gen_return_txouts()

    def run_test(self):
        txids = []
        utxos = create_confirmed_utxos(self.relayfee, self.nodes[0], 491)

        #create a mempool tx that will be evicted
        us0 = utxos.pop()
        inputs = [{ "txid" : us0["txid"], "vout" : us0["vout"]}]
        outputs = {self.nodes[0].getnewaddress() : 0.0001}
        tx = self.nodes[0].createrawtransaction(inputs, outputs)
        self.nodes[0].settxfee(self.relayfee) # specifically fund this tx with low fee
        txF = self.nodes[0].fundrawtransaction(tx)
        self.nodes[0].settxfee(0) # return to automatic fee selection
        txFS = self.nodes[0].signrawtransaction(txF['hex'])
        txid = self.nodes[0].sendrawtransaction(txFS['hex'])

        relayfee = self.nodes[0].getnetworkinfo()['relayfee']
        base_fee = relayfee*100
        for i in range (3):
            txids.append([])
            txids[i] = create_lots_of_big_transactions(self.nodes[0], self.txouts, utxos[30*i:30*i+30], 30, (i+1)*base_fee)

        # by now, the tx should be evicted, check confirmation state
        assert(txid not in self.nodes[0].getrawmempool())
        txdata = self.nodes[0].gettransaction(txid)
        assert(txdata['confirmations'] ==  0) #confirmation should still be 0

if __name__ == '__main__':
    MempoolLimitTest().main()
