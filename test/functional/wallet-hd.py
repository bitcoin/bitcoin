#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    start_nodes,
    start_node,
    assert_equal,
    connect_nodes_bi,
    assert_start_raises_init_error
)
import os
import shutil


class WalletHDTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-usehd=0'], ['-usehd=1', '-keypool=0']]

    def run_test (self):
        tmpdir = self.options.tmpdir

        # Make sure can't switch off usehd after wallet creation
        self.stop_node(1)
        assert_start_raises_init_error(1, self.options.tmpdir, ['-usehd=0'], 'already existing HD wallet')
        self.nodes[1] = start_node(1, self.options.tmpdir, self.extra_args[1])
        connect_nodes_bi(self.nodes, 0, 1)

        # Make sure we use hd, keep masterkeyid
        masterkeyid = self.nodes[1].getwalletinfo()['hdmasterkeyid']
        assert_equal(len(masterkeyid), 40)

        # create an internal key
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV= self.nodes[1].validateaddress(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/0'/1'/0'") #first internal child key

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
            assert_equal(hd_info["hdkeypath"], "m/0'/0'/"+str(i+1)+"'")
            assert_equal(hd_info["hdmasterkeyid"], masterkeyid)
            self.nodes[0].sendtoaddress(hd_add, 1)
            self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(non_hd_add, 1)
        self.nodes[0].generate(1)

        # create an internal key (again)
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV= self.nodes[1].validateaddress(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/0'/1'/1'") #second internal child key

        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), num_hd_adds + 1)

        self.log.info("Restore backup ...")
        self.stop_node(1)
        os.remove(self.options.tmpdir + "/node1/regtest/wallet.dat")
        shutil.copyfile(tmpdir + "/hd.bak", tmpdir + "/node1/regtest/wallet.dat")
        self.nodes[1] = start_node(1, self.options.tmpdir, self.extra_args[1])
        #connect_nodes_bi(self.nodes, 0, 1)

        # Assert that derivation is deterministic
        hd_add_2 = None
        for _ in range(num_hd_adds):
            hd_add_2 = self.nodes[1].getnewaddress()
            hd_info_2 = self.nodes[1].validateaddress(hd_add_2)
            assert_equal(hd_info_2["hdkeypath"], "m/0'/0'/"+str(_+1)+"'")
            assert_equal(hd_info_2["hdmasterkeyid"], masterkeyid)
        assert_equal(hd_add, hd_add_2)

        # Needs rescan
        self.stop_node(1)
        self.nodes[1] = start_node(1, self.options.tmpdir, self.extra_args[1] + ['-rescan'])
        #connect_nodes_bi(self.nodes, 0, 1)
        assert_equal(self.nodes[1].getbalance(), num_hd_adds + 1)

        # send a tx and make sure its using the internal chain for the changeoutput
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        outs = self.nodes[1].decoderawtransaction(self.nodes[1].gettransaction(txid)['hex'])['vout']
        keypath = ""
        for out in outs:
            if out['value'] != 1:
                keypath = self.nodes[1].validateaddress(out['scriptPubKey']['addresses'][0])['hdkeypath']
        
        assert_equal(keypath[0:7], "m/0'/1'")

if __name__ == '__main__':
    WalletHDTest().main ()
