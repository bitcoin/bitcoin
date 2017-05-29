#!/usr/bin/env python2
# coding=utf-8
# ^^^^^^^^^^^^ TODO remove when supporting only Python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class WalletHDTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self):
        self.nodes = start_nodes(2, self.options.tmpdir, [['-usehd=0'], ['-usehd=1', '-keypool=0']])
        self.is_network_split = False
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        tmpdir = self.options.tmpdir

        # Make sure can't switch off usehd after wallet creation
        stop_node(self.nodes[1],1)
        try:
            start_node(1, self.options.tmpdir, ['-usehd=0'])
            raise AssertionError("Must not allow to turn off HD on an already existing HD wallet")
        except Exception as e:
            assert("dashd exited with status 1 during initialization" in str(e))
        # assert_start_raises_init_error(1, self.options.tmpdir, ['-usehd=0'], 'already existing HD wallet')
        # self.nodes[1] = start_node(1, self.options.tmpdir, self.node_args[1])
        self.nodes[1] = start_node(1, self.options.tmpdir, ['-usehd=1', '-keypool=0'])
        connect_nodes_bi(self.nodes, 0, 1)

        # Make sure we use hd, keep chainid
        chainid = self.nodes[1].getwalletinfo()['hdchainid']
        assert_equal(len(chainid), 64)

        # create an internal key
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV= self.nodes[1].validateaddress(change_addr);
        assert_equal(change_addrV["hdkeypath"], "m/44'/1'/0'/1/0") #first internal child key

        # Import a non-HD private key in the HD wallet
        non_hd_add = self.nodes[0].getnewaddress()
        self.nodes[1].importprivkey(self.nodes[0].dumpprivkey(non_hd_add))

        # This should be enough to keep the master key and the non-HD key
        self.nodes[1].backupwallet(tmpdir + "/hd.bak")
        #self.nodes[1].dumpwallet(tmpdir + "/hd.dump")

        # Derive some HD addresses and remember the last
        # Also send funds to each add
        self.nodes[0].generate(101)
        hd_add = None
        num_hd_adds = 300
        for i in range(num_hd_adds):
            hd_add = self.nodes[1].getnewaddress()
            hd_info = self.nodes[1].validateaddress(hd_add)
            assert_equal(hd_info["hdkeypath"], "m/44'/1'/0'/0/"+str(i+1))
            assert_equal(hd_info["hdchainid"], chainid)
            self.nodes[0].sendtoaddress(hd_add, 1)
            self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(non_hd_add, 1)
        self.nodes[0].generate(1)

        # create an internal key (again)
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV= self.nodes[1].validateaddress(change_addr);
        assert_equal(change_addrV["hdkeypath"], "m/44'/1'/0'/1/1") #second internal child key

        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), num_hd_adds + 1)

        print("Restore backup ...")
        stop_node(self.nodes[1],1)
        os.remove(self.options.tmpdir + "/node1/regtest/wallet.dat")
        shutil.copyfile(tmpdir + "/hd.bak", tmpdir + "/node1/regtest/wallet.dat")
        self.nodes[1] = start_node(1, self.options.tmpdir, ['-usehd=1', '-keypool=0'])
        #connect_nodes_bi(self.nodes, 0, 1)

        # Assert that derivation is deterministic
        hd_add_2 = None
        for _ in range(num_hd_adds):
            hd_add_2 = self.nodes[1].getnewaddress()
            hd_info_2 = self.nodes[1].validateaddress(hd_add_2)
            assert_equal(hd_info_2["hdkeypath"], "m/44'/1'/0'/0/"+str(_+1))
            assert_equal(hd_info_2["hdchainid"], chainid)
        assert_equal(hd_add, hd_add_2)

        # Needs rescan
        stop_node(self.nodes[1],1)
        self.nodes[1] = start_node(1, self.options.tmpdir, ['-usehd=1', '-keypool=0', '-rescan'])
        #connect_nodes_bi(self.nodes, 0, 1)
        assert_equal(self.nodes[1].getbalance(), num_hd_adds + 1)

        # send a tx and make sure its using the internal chain for the changeoutput
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        outs = self.nodes[1].decoderawtransaction(self.nodes[1].gettransaction(txid)['hex'])['vout'];
        keypath = ""
        for out in outs:
            if out['value'] != 1:
                keypath = self.nodes[1].validateaddress(out['scriptPubKey']['addresses'][0])['hdkeypath']

        assert_equal(keypath[0:13], "m/44'/1'/0'/1")

if __name__ == '__main__':
    WalletHDTest().main ()
