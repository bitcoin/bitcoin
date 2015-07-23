#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Exercise the wallet.  Ported from wallet.sh.  
# Does the following:
#   a) creates 3 nodes, with an empty chain (no blocks).
#   b) node0 mines a block
#   c) node1 mines 101 blocks, so now nodes 0 and 1 have 50btc, node2 has none. 
#   d) node0 sends 21 btc to node2, in two transactions (11 btc, then 10 btc).
#   e) node0 mines a block, collects the fee on the second transaction
#   f) node1 mines 100 blocks, to mature node0's just-mined block
#   g) check that node0 has 100-21, node2 has 21
#   h) node0 should now have 2 unspent outputs;  send these to node2 via raw tx broadcast by node1
#   i) have node1 mine a block
#   j) check balances - node0 should have 0, node2 should have 100
#   k) test ResendWalletTransactions - create transactions, startup fourth node, make sure it syncs
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class WalletTest (BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self, split=False):
        

        self.nodes = start_nodes(3, self.options.tmpdir)
        
        #connect to a local machine for debugging
        # url = "http://bitcoinrpc:DP6DvqZtqXarpeNWyN3LZTFchCCyCUuHwNF7E8pX99x1@%s:%d" % ('127.0.0.1', 18332)
        # proxy = AuthServiceProxy(url)
        # proxy.url = url # store URL on proxy for info
        # self.nodes.append(proxy)
        
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        print "Mining blocks..."

        encrypt = True
        self.nodes[0].generate(1)

        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 50)
        assert_equal(walletinfo['balance'], 0)
        self.nodes[0].generate(100)
        self.sync_all()

        self.nodes[2].hdaddchain('default', 'f81a7a4efdc29e54dcc739df87315a756038d0b68fbc4880ffbbbef222152e6a')
        adr = self.nodes[2].hdgetaddress()
        assert_equal(adr['address'], "n1hBoYyGjqkbC8kdKNAejuaNR19eoYCSoi");
        assert_equal(adr['chainpath'], "m/44'/0'/0'/0/0");
        
        adr2 = self.nodes[2].hdgetaddress()
        assert_equal(adr2['address'], "mvFePVSFGELgCDyLYrTJJ3tijnyeB9UF6p");
        assert_equal(adr2['chainpath'], "m/44'/0'/0'/0/1");

        self.nodes[0].sendtoaddress(adr['address'], 11);
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()
        walletinfo = self.nodes[2].getwalletinfo()
        assert_equal(walletinfo['balance'], 11)
            
        stop_node(self.nodes[0], 0)
        stop_node(self.nodes[1], 1)
        stop_node(self.nodes[2], 2)

        #try to cover over master seed
        os.remove(self.options.tmpdir + "/node2/regtest/wallet.dat")
        self.nodes[2] = start_node(2, self.options.tmpdir)
        
        if encrypt:
            print "encrypt wallet"
            self.nodes[2].encryptwallet("test")
            bitcoind_processes[2].wait()
            del bitcoind_processes[2]
            
            self.nodes[2] = start_node(2, self.options.tmpdir)
            self.nodes[2].walletpassphrase("test", 100)
            
        self.nodes[2].hdaddchain('default', 'f81a7a4efdc29e54dcc739df87315a756038d0b68fbc4880ffbbbef222152e6a')
        #generate address
        adr = self.nodes[2].hdgetaddress()
        assert_equal(adr['address'], "n1hBoYyGjqkbC8kdKNAejuaNR19eoYCSoi"); #must be deterministic
        walletinfo = self.nodes[2].getwalletinfo()
        assert_equal(walletinfo['balance'], 0) #balance should be o beause we need to rescan first

        stop_node(self.nodes[2], 2)
        self.nodes[0] = start_node(0, self.options.tmpdir)
        self.nodes[1] = start_node(1, self.options.tmpdir)
        self.nodes[2] = start_node(2, self.options.tmpdir, ['-rescan=1'])
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        
        walletinfo = self.nodes[2].getwalletinfo()
        assert_equal(walletinfo['balance'], 11) #after rescan we should have detected the spendable coins

        walletinfo = self.nodes[0].getwalletinfo()
        balanceOld = walletinfo['balance'];
        
        if encrypt:
            self.nodes[2].walletpassphrase("test", 100)
            
        self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 2.0); #try to send (sign) with HD keymaterial
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()
        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['balance'], balanceOld+Decimal('52.00000000'))

if __name__ == '__main__':
    WalletTest ().main ()
