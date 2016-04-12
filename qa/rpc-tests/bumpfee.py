#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class BumpFeeTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 3
        self.setup_clean_chain = True

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        print("Mining blocks...")

        self.nodes[0].generate(1)

        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 50)
        assert_equal(walletinfo['balance'], 0)

        self.sync_all()
        self.nodes[1].generate(101)
        self.sync_all()

        assert_equal(self.nodes[0].getbalance(), 50)
        assert_equal(self.nodes[1].getbalance(), 50)
        assert_equal(self.nodes[2].getbalance(), 0)

        # create a opt-in RBF transaction
        rbftx = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 5, "", "", True, True)
        rbftxraw = self.nodes[0].gettransaction(rbftx)
        bumpedrawtx = self.nodes[0].bumpfee(rbftx)
        assert(bumpedrawtx['fee'] - rbftxraw['fee'] > 0) #make sure the fee is at least higher

        rbftxrawsignedN2= self.nodes[0].getrawtransaction(bumpedrawtx['txid'])
        sync_mempools(self.nodes)
        rbftxrawsignedN0= self.nodes[1].getrawtransaction(bumpedrawtx['txid'])
        assert_equal(rbftxrawsignedN2, rbftxrawsignedN0) #node0 should have the transacion
        self.sync_all()

        # try to get the replaced transaction, should be evicted from the mempool
        assert_raises(JSONRPCException, self.nodes[1].getrawtransaction, rbftx)

        oldwtx = self.nodes[0].gettransaction(rbftx)
        assert(len(oldwtx['walletconflicts']) > 0)
        assert(rbftx not in self.nodes[0].getrawmempool())

        self.nodes[1].generate(1)
        self.sync_all()

        rbftx = self.nodes[2].sendmany('', {self.nodes[1].getnewaddress() : 1, self.nodes[1].getnewaddress() : 2}, 0, "", [], True)
        bumpedrawtx = self.nodes[2].bumpfee(rbftx)
        self.sync_all()

        assert(rbftx not in self.nodes[0].getrawmempool())
        assert(bumpedrawtx['txid'] in self.nodes[0].getrawmempool())

        # try to replace a non RBF transaction
        nonrbftx = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1, "", "", True, False)

        try:
            bumpedrawtx = self.nodes[0].bumpfee(nonrbftx)
            assert(False)
        except JSONRPCException as e:
            assert(len(e.error['message']) > 0)

if __name__ == '__main__':
    BumpFeeTest ().main ()
