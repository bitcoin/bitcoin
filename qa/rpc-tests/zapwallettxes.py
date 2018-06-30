#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class ZapWalletTXesTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        print("Mining blocks...")
        self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[1].generate(101)
        self.sync_all()
        
        assert_equal(self.nodes[0].getbalance(), 500)
        
        txid0 = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        txid1 = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 10)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        
        txid2 = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        txid3 = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 10)
        
        tx0 = self.nodes[0].gettransaction(txid0)
        assert_equal(tx0['txid'], txid0) #tx0 must be available (confirmed)
        
        tx1 = self.nodes[0].gettransaction(txid1)
        assert_equal(tx1['txid'], txid1) #tx1 must be available (confirmed)
        
        tx2 = self.nodes[0].gettransaction(txid2)
        assert_equal(tx2['txid'], txid2) #tx2 must be available (unconfirmed)
        
        tx3 = self.nodes[0].gettransaction(txid3)
        assert_equal(tx3['txid'], txid3) #tx3 must be available (unconfirmed)
        
        #restart bitcoind
        self.nodes[0].stop()
        bitcoind_processes[0].wait()
        self.nodes[0] = start_node(0,self.options.tmpdir)
        
        tx3 = self.nodes[0].gettransaction(txid3)
        assert_equal(tx3['txid'], txid3) #tx must be available (unconfirmed)
        
        self.nodes[0].stop()
        bitcoind_processes[0].wait()
        
        #restart bitcoind with zapwallettxes
        self.nodes[0] = start_node(0,self.options.tmpdir, ["-zapwallettxes=1"])
        
        assert_raises(JSONRPCException, self.nodes[0].gettransaction, [txid3])
        #there must be a expection because the unconfirmed wallettx0 must be gone by now

        tx0 = self.nodes[0].gettransaction(txid0)
        assert_equal(tx0['txid'], txid0) #tx0 (confirmed) must still be available because it was confirmed


if __name__ == '__main__':
    ZapWalletTXesTest ().main ()
