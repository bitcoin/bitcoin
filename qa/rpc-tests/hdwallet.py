#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class HDWalletTest (BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self, split=False):
        self.nodes = start_nodes(2, self.options.tmpdir, extra_args=[
            ['-hdseed=f81a7a4efdc29e54dcc739df87315a756038d0b68fbc4880ffbbbef222152e6a'],
            []
            ])
        connect_nodes_bi(self.nodes,0,1)
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

        adr = self.nodes[0].getnewaddress("", True)
        assert_equal(adr['address'], "mtGrK6eX8uhW6FhoUzyQmxFBLxjZRhcfQJ");
        assert_equal(adr['keypath'], "m/0'/3'");
        
        adr = self.nodes[0].getnewaddress("", True)
        assert_equal(adr['address'], "muWGGSSma5s7TjHbT5fCKunkoyBaF1uy8D");
        assert_equal(adr['keypath'], "m/0'/4'");
        
        stop_node(self.nodes[0], 0)
        stop_node(self.nodes[1], 1)
        
        #try to recover over master seed
        os.remove(self.options.tmpdir + "/node0/regtest/wallet.dat")
        self.nodes = start_nodes(2, self.options.tmpdir, extra_args=[
            ['-hdseed=f81a7a4efdc29e54dcc739df87315a756038d0b68fbc4880ffbbbef222152e6a'],
            []
            ])
        adr = self.nodes[0].getnewaddress("", True)
        assert_equal(adr['address'], "mqfCLB8d4vP1BTyMo88WzjT9VJG4NWEbni");
        assert_equal(adr['keypath'], "m/0'/1'");
        
        adr = self.nodes[0].getnewaddress("", True)
        assert_equal(adr['address'], "n4mEdLhWJgDHvsChPcttAkqQSMrndhzdAv");
        assert_equal(adr['keypath'], "m/0'/2'");

        adr = self.nodes[0].getnewaddress("", True)
        assert_equal(adr['address'], "mtGrK6eX8uhW6FhoUzyQmxFBLxjZRhcfQJ");
        assert_equal(adr['keypath'], "m/0'/3'");
        
        print "encrypt wallet"
        self.nodes[0].encryptwallet("test")
        bitcoind_processes[0].wait()
        del bitcoind_processes[0]

        self.nodes[0] = start_node(0, self.options.tmpdir, extra_args=['-hdseed=f81a7a4efdc29e54dcc739df87315a756038d0b68fbc4880ffbbbef222152e6a'])
        self.nodes[0].walletpassphrase("test", 100)
        adr = self.nodes[0].getnewaddress("", True)
        assert_equal(adr['address'], "n32WRiXX6P6KFkdGV37CAbsTjLcxt4VhRY");
        assert_equal(adr['keypath'], "m/0'/5'");

if __name__ == '__main__':
    HDWalletTest ().main ()
