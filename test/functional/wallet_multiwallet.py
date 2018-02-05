#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiwallet.

Verify that a bitcoind node can load multiple wallet files
"""
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class MultiWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-wallet=w1', '-wallet=w2', '-wallet=w3', '-wallet=w'], []]
        self.supports_cli = True

    def run_test(self):
        node = self.nodes[0]

        data_dir = lambda *p: os.path.join(node.datadir, 'regtest', *p)
        wallet_dir = lambda *p: data_dir('wallets', *p)
        wallet = lambda name: node.get_wallet_rpc(name)

        assert_equal(set(node.listwallets()), {"w1", "w2", "w3", "w"})

        self.stop_nodes()

        self.assert_start_raises_init_error(0, ['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" does not exist')
        self.assert_start_raises_init_error(0, ['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" is a relative path', cwd=data_dir())
        self.assert_start_raises_init_error(0, ['-walletdir=debug.log'], 'Error: Specified -walletdir "debug.log" is not a directory', cwd=data_dir())

        # should not initialize if there are duplicate wallets
        self.assert_start_raises_init_error(0, ['-wallet=w1', '-wallet=w1'], 'Error loading wallet w1. Duplicate -wallet filename specified.')

        # should not initialize if wallet file is a directory
        os.mkdir(wallet_dir('w11'))
        self.assert_start_raises_init_error(0, ['-wallet=w11'], 'Error loading wallet w11. -wallet filename must be a regular file.')

        # should not initialize if one wallet is a copy of another
        shutil.copyfile(wallet_dir('w2'), wallet_dir('w22'))
        self.assert_start_raises_init_error(0, ['-wallet=w2', '-wallet=w22'], 'duplicates fileid')

        # should not initialize if wallet file is a symlink
        os.symlink(wallet_dir('w1'), wallet_dir('w12'))
        self.assert_start_raises_init_error(0, ['-wallet=w12'], 'Error loading wallet w12. -wallet filename must be a regular file.')

        # should not initialize if the specified walletdir does not exist
        self.assert_start_raises_init_error(0, ['-walletdir=bad'], 'Error: Specified -walletdir "bad" does not exist')
        # should not initialize if the specified walletdir is not a directory
        not_a_dir = wallet_dir('notadir')
        open(not_a_dir, 'a').close()
        self.assert_start_raises_init_error(0, ['-walletdir=' + not_a_dir], 'Error: Specified -walletdir "' + not_a_dir + '" is not a directory')

        # if wallets/ doesn't exist, datadir should be the default wallet dir
        wallet_dir2 = data_dir('walletdir')
        os.rename(wallet_dir(), wallet_dir2)
        self.start_node(0, ['-wallet=w4', '-wallet=w5'])
        assert_equal(set(node.listwallets()), {"w4", "w5"})
        w5 = wallet("w5")
        w5.generate(1)

        # now if wallets/ exists again, but the rootdir is specified as the walletdir, w4 and w5 should still be loaded
        os.rename(wallet_dir2, wallet_dir())
        self.restart_node(0, ['-wallet=w4', '-wallet=w5', '-walletdir=' + data_dir()])
        assert_equal(set(node.listwallets()), {"w4", "w5"})
        w5 = wallet("w5")
        w5_info = w5.getwalletinfo()
        assert_equal(w5_info['immature_balance'], 50)

        competing_wallet_dir = os.path.join(self.options.tmpdir, 'competing_walletdir')
        os.mkdir(competing_wallet_dir)
        self.restart_node(0, ['-walletdir='+competing_wallet_dir])
        self.assert_start_raises_init_error(1, ['-walletdir='+competing_wallet_dir], 'Error initializing wallet database environment')

        self.restart_node(0, self.extra_args[0])

        w1 = wallet("w1")
        w2 = wallet("w2")
        w3 = wallet("w3")
        w4 = wallet("w")
        wallet_bad = wallet("bad")

        w1.generate(1)

        # accessing invalid wallet fails
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded", wallet_bad.getwalletinfo)

        # accessing wallet RPC without using wallet endpoint fails
        assert_raises_rpc_error(-19, "Wallet file not specified", node.getwalletinfo)

        # check w1 wallet balance
        w1_info = w1.getwalletinfo()
        assert_equal(w1_info['immature_balance'], 50)
        w1_name = w1_info['walletname']
        assert_equal(w1_name, "w1")

        # check w2 wallet balance
        w2_info = w2.getwalletinfo()
        assert_equal(w2_info['immature_balance'], 0)
        w2_name = w2_info['walletname']
        assert_equal(w2_name, "w2")

        w3_name = w3.getwalletinfo()['walletname']
        assert_equal(w3_name, "w3")

        w4_name = w4.getwalletinfo()['walletname']
        assert_equal(w4_name, "w")

        w1.generate(101)
        assert_equal(w1.getbalance(), 100)
        assert_equal(w2.getbalance(), 0)
        assert_equal(w3.getbalance(), 0)
        assert_equal(w4.getbalance(), 0)

        w1.sendtoaddress(w2.getnewaddress(), 1)
        w1.sendtoaddress(w3.getnewaddress(), 2)
        w1.sendtoaddress(w4.getnewaddress(), 3)
        w1.generate(1)
        assert_equal(w2.getbalance(), 1)
        assert_equal(w3.getbalance(), 2)
        assert_equal(w4.getbalance(), 3)

        batch = w1.batch([w1.getblockchaininfo.get_request(), w1.getwalletinfo.get_request()])
        assert_equal(batch[0]["result"]["chain"], "regtest")
        assert_equal(batch[1]["result"]["walletname"], "w1")

if __name__ == '__main__':
    MultiWalletTest().main()
