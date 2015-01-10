#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework import BitcoinTestFramework
from util import *


class ZapWalletTXesTest (BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        print "Mining blocks..."
        self.nodes[0].setgenerate(True, 1)
        self.sync_all()
        self.nodes[1].setgenerate(True, 101)
        self.sync_all()
        
        assert_equal(self.nodes[0].getbalance(), 50)
        
        txid0 = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        txid1 = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 10)
        self.sync_all()
        self.nodes[0].setgenerate(True, 1)
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
        
        aException = False
        try:
            tx3 = self.nodes[0].gettransaction(txid3)
        except JSONRPCException,e:
            print e
            aException = True
        
        assert_equal(aException, True) #there must be a expection because the unconfirmed wallettx0 must be gone by now

        tx0 = self.nodes[0].gettransaction(txid0)
        assert_equal(tx0['txid'], txid0) #tx0 (confirmed) must still be available because it was confirmed


if __name__ == '__main__':
    ZapWalletTXesTest ().main ()
