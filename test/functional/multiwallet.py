#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
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
        self.num_nodes = 1
        self.extra_args = [['-wallet=w1', '-wallet=w2', '-wallet=w3']]

    def run_test(self):
        assert_equal(set(self.nodes[0].listwallets()), {"w1", "w2", "w3"})

        self.stop_node(0)

        # should not initialize if there are duplicate wallets
        self.assert_start_raises_init_error(0, ['-wallet=w1', '-wallet=w1'], 'Error loading wallet w1. Duplicate -wallet filename specified.')

        # should not initialize if wallet file is a directory
        os.mkdir(os.path.join(self.options.tmpdir, 'node0', 'regtest', 'w11'))
        self.assert_start_raises_init_error(0, ['-wallet=w11'], 'Error loading wallet w11. -wallet filename must be a regular file.')

        # should not initialize if one wallet is a copy of another
        shutil.copyfile(os.path.join(self.options.tmpdir, 'node0', 'regtest', 'w2'),
                        os.path.join(self.options.tmpdir, 'node0', 'regtest', 'w22'))
        self.assert_start_raises_init_error(0, ['-wallet=w2', '-wallet=w22'], 'duplicates fileid')

        # should not initialize if wallet file is a symlink
        os.symlink(os.path.join(self.options.tmpdir, 'node0', 'regtest', 'w1'), os.path.join(self.options.tmpdir, 'node0', 'regtest', 'w12'))
        self.assert_start_raises_init_error(0, ['-wallet=w12'], 'Error loading wallet w12. -wallet filename must be a regular file.')

        self.start_node(0, self.extra_args[0])

        w1 = self.nodes[0].get_wallet_rpc("w1")
        w2 = self.nodes[0].get_wallet_rpc("w2")
        w3 = self.nodes[0].get_wallet_rpc("w3")
        wallet_bad = self.nodes[0].get_wallet_rpc("bad")

        w1.generate(1)

        # accessing invalid wallet fails
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded", wallet_bad.getwalletinfo)

        # accessing wallet RPC without using wallet endpoint fails
        assert_raises_rpc_error(-19, "Wallet file not specified", self.nodes[0].getwalletinfo)

        # check w1 wallet balance
        w1_info = w1.getwalletinfo()
        assert_equal(w1_info['immature_balance'], 500)
        w1_name = w1_info['walletname']
        assert_equal(w1_name, "w1")

        # check w2 wallet balance
        w2_info = w2.getwalletinfo()
        assert_equal(w2_info['immature_balance'], 0)
        w2_name = w2_info['walletname']
        assert_equal(w2_name, "w2")

        w3_name = w3.getwalletinfo()['walletname']
        assert_equal(w3_name, "w3")

        assert_equal({"w1", "w2", "w3"}, {w1_name, w2_name, w3_name})

        w1.generate(101)
        assert_equal(w1.getbalance(), 1000)
        assert_equal(w2.getbalance(), 0)
        assert_equal(w3.getbalance(), 0)

        w1.sendtoaddress(w2.getnewaddress(), 1)
        w1.sendtoaddress(w3.getnewaddress(), 2)
        w1.generate(1)
        assert_equal(w2.getbalance(), 1)
        assert_equal(w3.getbalance(), 2)

        batch = w1.batch([w1.getblockchaininfo.get_request(), w1.getwalletinfo.get_request()])
        assert_equal(batch[0]["result"]["chain"], "regtest")
        assert_equal(batch[1]["result"]["walletname"], "w1")

if __name__ == '__main__':
    MultiWalletTest().main()
