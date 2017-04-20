#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    start_nodes,
    start_node,
    assert_equal,
    connect_nodes_bi,
    assert_start_raises_init_error,
    sync_blocks
)
import os
import shutil


class WalletHDRestoreTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.node_args = [['-usehd=0'], ['-usehd=1', '-keypool=100']]

    def setup_network(self):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.node_args)
        self.is_network_split = False
        connect_nodes_bi(self.nodes, 0, 1)

    def run_test (self):
        tmpdir = self.options.tmpdir
        self.stop_node(1)
        shutil.copyfile(tmpdir + "/node1/regtest/wallet.dat", tmpdir + "/hd.bak")
        self.nodes[1] = start_node(1, self.options.tmpdir, self.node_args[1])
        connect_nodes_bi(self.nodes,0,1)
        for _ in range(10):
            addr = self.nodes[1].getnewaddress()
            
        self.nodes[0].generate(101)
        addr = self.nodes[1].getnewaddress()
        assert_equal(self.nodes[1].validateaddress(addr)['hdkeypath'], "m/0'/0'/11'")
        
        rawch = self.nodes[1].getrawchangeaddress()
        txid = self.nodes[0].sendtoaddress(addr, 1)
        n0addr = self.nodes[0].getnewaddress()
        txdata = self.nodes[0].createrawtransaction([], {rawch : 2.0, n0addr: 3.0})

        txdata_f = self.nodes[0].fundrawtransaction(txdata)
        txdata_s = self.nodes[0].signrawtransaction(txdata_f['hex'])
        txid = self.nodes[0].sendrawtransaction(txdata_s['hex'])

        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        assert_equal(self.nodes[1].getbalance(), 3)
        assert_equal(self.nodes[1].listtransactions()[0]['category'], "receive")
        
        self.stop_node(1)
        shutil.rmtree(tmpdir + "/node1/regtest")
        os.mkdir(tmpdir + "/node1/regtest")
        shutil.copyfile(tmpdir + "/hd.bak", tmpdir + "/node1/regtest/wallet.dat")
        self.nodes[0].generate(1)
        self.nodes[1] = start_node(1, self.options.tmpdir, self.node_args[1])
        connect_nodes_bi(self.nodes,0,1)
        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), 3) #make sure we have reconstructed the transaction
        assert_equal(self.nodes[1].listtransactions()[0]['category'], "receive")
        
        #now check if we have marked all keys up to the used keypool key as used
        assert_equal(self.nodes[1].validateaddress(self.nodes[1].getnewaddress())['hdkeypath'], "m/0'/0'/12'")
        
        #make sure the key on the internal chain is also marked as used
        assert_equal(self.nodes[1].validateaddress(self.nodes[1].getrawchangeaddress())['hdkeypath'], "m/0'/1'/1'")
        


if __name__ == '__main__':
    WalletHDRestoreTest().main ()
