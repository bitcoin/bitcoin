#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import os
import shutil
from pprint import pprint

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

        print("Initialize wallet including backups of unencrypted and encrypted wallet")
        # stop and backup original wallet (only keypool has been initialized)
        self.stop_node(1)
        shutil.copyfile(tmpdir + "/node1/regtest/wallet.dat", tmpdir + "/hd.bak")

        # start again and encrypt wallet
        self.nodes[1] = start_node(1, self.options.tmpdir, self.node_args[1])
        self.nodes[1].encryptwallet('test')
        bitcoind_processes[1].wait()
        # node will be stopped during encryption, now do a backup
        shutil.copyfile(tmpdir + "/node1/regtest/wallet.dat", tmpdir + "/hd.enc.bak") 

        # start the node with encrypted wallet, get address in new pool at pos 50 (over the gap limit)
        self.nodes[1] = start_node(1, self.options.tmpdir, self.node_args[1])
        for _ in range(50):
            addr_enc_oldpool = self.nodes[1].getnewaddress()

        # now make sure we retrive an address in the extended pool
        self.nodes[1].walletpassphrase("test", 10)
        for _ in range(80):
            addr_enc_extpool = self.nodes[1].getnewaddress()

        # stop and load initial backup of the unencrypted wallet
        self.stop_node(1)
        os.remove(tmpdir + "/node1/regtest/wallet.dat")
        shutil.copyfile(tmpdir + "/hd.bak", tmpdir + "/node1/regtest/wallet.dat")
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

        print("Testing with unencrypted wallet")
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

        # continue send funds (one in the main keypool over the gap limit, the other in the extended pool space)
        txid = self.nodes[0].sendtoaddress(addr_enc_oldpool, 10)
        self.nodes[0].generate(1)
        stop_height = self.nodes[0].getblockchaininfo()['blocks']
        txid = self.nodes[0].sendtoaddress(addr_enc_extpool, 5)
        self.nodes[0].generate(1)

        #########################################################
        #########################################################

        print("Testing with encrypted wallet")
        # Try with the encrypted wallet (non pruning)
        self.stop_node(1)
        shutil.rmtree(tmpdir + "/node1/regtest")
        os.mkdir(tmpdir + "/node1/regtest")
        shutil.copyfile(tmpdir + "/hd.enc.bak", tmpdir + "/node1/regtest/wallet.dat")
        self.nodes[1] = start_node(1, self.options.tmpdir, ['-usehd=1', '-keypool=100'])
        connect_nodes_bi(self.nodes,0,1)

        # Sync must be possible, though the wallet bestblock should lack behind
        self.sync_all()

        # The balance should cover everything expect the very last tx of 5 BTC
        assert_equal(self.nodes[1].getbalance(), 13)

        # unlock the wallet, the sync can continue then
        self.nodes[1].walletpassphrase("test", 100)
        self.sync_all() #sync is now possible

        # we should now have restored all funds
        assert_equal(self.nodes[1].getbalance(), 18) # all funds recovered
        assert_equal(self.nodes[1].listtransactions()[0]['category'], "receive")

        #########################################################
        #########################################################

        print("Testing with encrypted wallet in prune mode")
        # Now try with the encrypted wallet in prune mode
        self.stop_node(1)
        shutil.rmtree(tmpdir + "/node1/regtest")
        os.mkdir(tmpdir + "/node1/regtest")
        shutil.copyfile(tmpdir + "/hd.enc.bak", tmpdir + "/node1/regtest/wallet.dat")
        self.nodes[1] = start_node(1, self.options.tmpdir, ['-usehd=1', '-keypool=100', '-prune=550'])
        connect_nodes_bi(self.nodes,0,1)

        # now we should only be capable to sync up to the second last block (pruned mode, sync will be paused)
        assert_equal(self.nodes[1].waitforblockheight(stop_height, 10 * 1000)['height'], stop_height) #must be possible

        # This must timeout now, we can't sync up to stop_height+1 (== most recent block)
        # Sync must be paused at this point
        assert_equal(self.nodes[1].waitforblockheight(stop_height+1, 3 * 1000)['height'], stop_height)

        # The balance should cover everything expect the very last tx of 5 BTC
        assert_equal(self.nodes[1].getbalance(), 13)

        # unlock the wallet, the sync can continue then
        self.nodes[1].walletpassphrase("test", 100)
        self.sync_all() #sync is now possible

        # we should now have restored all funds
        assert_equal(self.nodes[1].getbalance(), 18)
        assert_equal(self.nodes[1].listtransactions()[0]['category'], "receive")

if __name__ == '__main__':
    WalletHDRestoreTest().main ()
