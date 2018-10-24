#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiwallet.

Verify that a dashd node can load multiple wallet files
"""
import os
import shutil
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class MultiWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.supports_cli = True

    def run_test(self):
        node = self.nodes[0]

        data_dir = lambda *p: os.path.join(node.datadir, self.chain, *p)
        wallet_dir = lambda *p: data_dir('wallets', *p)
        wallet = lambda name: node.get_wallet_rpc(name)

        # check wallet.dat is created
        self.stop_nodes()
        assert_equal(os.path.isfile(wallet_dir('wallet.dat')), True)

        # create symlink to verify wallet directory path can be referenced
        # through symlink
        os.mkdir(wallet_dir('w7'))
        os.symlink('w7', wallet_dir('w7_symlink'))

        # rename wallet.dat to make sure plain wallet file paths (as opposed to
        # directory paths) can be loaded
        os.rename(wallet_dir("wallet.dat"), wallet_dir("w8"))

        # restart node with a mix of wallet names:
        #   w1, w2, w3 - to verify new wallets created when non-existing paths specified
        #   w          - to verify wallet name matching works when one wallet path is prefix of another
        #   sub/w5     - to verify relative wallet path is created correctly
        #   extern/w6  - to verify absolute wallet path is created correctly
        #   w7_symlink - to verify symlinked wallet path is initialized correctly
        #   w8         - to verify existing wallet file is loaded correctly
        #   ''         - to verify default wallet file is created correctly
        wallet_names = ['w1', 'w2', 'w3', 'w', 'sub/w5', os.path.join(self.options.tmpdir, 'extern/w6'), 'w7_symlink', 'w8', '']
        extra_args = ['-wallet={}'.format(n) for n in wallet_names]
        self.start_node(0, extra_args)
        assert_equal(set(node.listwallets()), set(wallet_names))

        # check that all requested wallets were created
        self.stop_node(0)
        for wallet_name in wallet_names:
            if os.path.isdir(wallet_dir(wallet_name)):
                assert_equal(os.path.isfile(wallet_dir(wallet_name, "wallet.dat")), True)
            else:
                assert_equal(os.path.isfile(wallet_dir(wallet_name)), True)

        # should not initialize if wallet path can't be created
        exp_stderr = "boost::filesystem::create_directory: (The system cannot find the path specified|Not a directory):"
        self.nodes[0].assert_start_raises_init_error(['-wallet=wallet.dat/bad'], exp_stderr, match=ErrorMatch.PARTIAL_REGEX)

        self.nodes[0].assert_start_raises_init_error(['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" does not exist')
        self.nodes[0].assert_start_raises_init_error(['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" is a relative path', cwd=data_dir())
        self.nodes[0].assert_start_raises_init_error(['-walletdir=debug.log'], 'Error: Specified -walletdir "debug.log" is not a directory', cwd=data_dir())

        # should not initialize if there are duplicate wallets
        self.nodes[0].assert_start_raises_init_error(['-wallet=w1', '-wallet=w1'], 'Error: Error loading wallet w1. Duplicate -wallet filename specified.')

        # should not initialize if one wallet is a copy of another
        shutil.copyfile(wallet_dir('w8'), wallet_dir('w8_copy'))
        exp_stderr = "Can't open database w8_copy \(duplicates fileid \w+ from w8\)"
        self.nodes[0].assert_start_raises_init_error(['-wallet=w8', '-wallet=w8_copy'], exp_stderr, match=ErrorMatch.PARTIAL_REGEX)

        # should not initialize if wallet file is a symlink
        os.symlink('w8', wallet_dir('w8_symlink'))
        self.nodes[0].assert_start_raises_init_error(['-wallet=w8_symlink'], 'Error: Invalid -wallet path \'w8_symlink\'\. .*', match=ErrorMatch.FULL_REGEX)

        # should not initialize if the specified walletdir does not exist
        self.nodes[0].assert_start_raises_init_error(['-walletdir=bad'], 'Error: Specified -walletdir "bad" does not exist')
        # should not initialize if the specified walletdir is not a directory
        not_a_dir = wallet_dir('notadir')
        open(not_a_dir, 'a', encoding="utf8").close()
        self.nodes[0].assert_start_raises_init_error(['-walletdir=' + not_a_dir], 'Error: Specified -walletdir "' + not_a_dir + '" is not a directory')

        self.log.info("Do not allow -zapwallettxes with multiwallet")
        self.nodes[0].assert_start_raises_init_error(['-zapwallettxes', '-wallet=w1', '-wallet=w2'], "Error: -zapwallettxes is only allowed with a single wallet file")
        self.nodes[0].assert_start_raises_init_error(['-zapwallettxes=1', '-wallet=w1', '-wallet=w2'], "Error: -zapwallettxes is only allowed with a single wallet file")
        self.nodes[0].assert_start_raises_init_error(['-zapwallettxes=2', '-wallet=w1', '-wallet=w2'], "Error: -zapwallettxes is only allowed with a single wallet file")

        self.log.info("Do not allow -salvagewallet with multiwallet")
        self.nodes[0].assert_start_raises_init_error(['-salvagewallet', '-wallet=w1', '-wallet=w2'], "Error: -salvagewallet is only allowed with a single wallet file")
        self.nodes[0].assert_start_raises_init_error(['-salvagewallet=1', '-wallet=w1', '-wallet=w2'], "Error: -salvagewallet is only allowed with a single wallet file")

        self.log.info("Do not allow -upgradewallet with multiwallet")
        self.nodes[0].assert_start_raises_init_error(['-upgradewallet', '-wallet=w1', '-wallet=w2'], "Error: -upgradewallet is only allowed with a single wallet file")
        self.nodes[0].assert_start_raises_init_error(['-upgradewallet=1', '-wallet=w1', '-wallet=w2'], "Error: -upgradewallet is only allowed with a single wallet file")

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
        assert_equal(w5_info['immature_balance'], 500)

        competing_wallet_dir = os.path.join(self.options.tmpdir, 'competing_walletdir')
        os.mkdir(competing_wallet_dir)
        self.restart_node(0, ['-walletdir=' + competing_wallet_dir])
        exp_stderr = "Error: Error initializing wallet database environment \"\S+competing_walletdir\"!"
        self.nodes[1].assert_start_raises_init_error(['-walletdir=' + competing_wallet_dir], exp_stderr, match=ErrorMatch.PARTIAL_REGEX)

        self.restart_node(0, extra_args)

        wallets = [wallet(w) for w in wallet_names]
        wallet_bad = wallet("bad")

        # check wallet names and balances
        wallets[0].generate(1)
        for wallet_name, wallet in zip(wallet_names, wallets):
            info = wallet.getwalletinfo()
            assert_equal(info['immature_balance'], 500 if wallet is wallets[0] else 0)
            assert_equal(info['walletname'], wallet_name)

        # accessing invalid wallet fails
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded", wallet_bad.getwalletinfo)

        # accessing wallet RPC without using wallet endpoint fails
        assert_raises_rpc_error(-19, "Wallet file not specified", node.getwalletinfo)

        w1, w2, w3, w4, *_ = wallets
        w1.generate(101)
        assert_equal(w1.getbalance(), 1000)
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
        assert_equal(batch[0]["result"]["chain"], self.chain)
        assert_equal(batch[1]["result"]["walletname"], "w1")

        self.log.info('Check for per-wallet settxfee call')
        assert_equal(w1.getwalletinfo()['paytxfee'], 0)
        assert_equal(w2.getwalletinfo()['paytxfee'], 0)
        w2.settxfee(4.0)
        assert_equal(w1.getwalletinfo()['paytxfee'], 0)
        assert_equal(w2.getwalletinfo()['paytxfee'], 4.0)

        self.log.info("Test dynamic wallet loading")

        self.restart_node(0, ['-nowallet'])
        assert_equal(node.listwallets(), [])
        assert_raises_rpc_error(-32601, "Method not found", node.getwalletinfo)

        self.log.info("Load first wallet")
        loadwallet_name = node.loadwallet(wallet_names[0])
        assert_equal(loadwallet_name['name'], wallet_names[0])
        assert_equal(node.listwallets(), wallet_names[0:1])
        node.getwalletinfo()
        w1 = node.get_wallet_rpc(wallet_names[0])
        w1.getwalletinfo()

        self.log.info("Load second wallet")
        loadwallet_name = node.loadwallet(wallet_names[1])
        assert_equal(loadwallet_name['name'], wallet_names[1])
        assert_equal(node.listwallets(), wallet_names[0:2])
        assert_raises_rpc_error(-19, "Wallet file not specified", node.getwalletinfo)
        w2 = node.get_wallet_rpc(wallet_names[1])
        w2.getwalletinfo()

        self.log.info("Load remaining wallets")
        for wallet_name in wallet_names[2:]:
            loadwallet_name = self.nodes[0].loadwallet(wallet_name)
            assert_equal(loadwallet_name['name'], wallet_name)

        assert_equal(set(self.nodes[0].listwallets()), set(wallet_names))

        # Fail to load if wallet doesn't exist
        assert_raises_rpc_error(-18, 'Wallet wallets not found.', self.nodes[0].loadwallet, 'wallets')

        # Fail to load duplicate wallets
        assert_raises_rpc_error(-4, 'Wallet file verification failed: Error loading wallet w1. Duplicate -wallet filename specified.', self.nodes[0].loadwallet, wallet_names[0])

        # Fail to load duplicate wallets by different ways (directory and filepath)
        assert_raises_rpc_error(-4, "Wallet file verification failed: Error loading wallet wallet.dat. Duplicate -wallet filename specified.", self.nodes[0].loadwallet, 'wallet.dat')

        # Fail to load if one wallet is a copy of another
        assert_raises_rpc_error(-1, "BerkeleyBatch: Can't open database w8_copy (duplicates fileid", self.nodes[0].loadwallet, 'w8_copy')

        # Fail to load if one wallet is a copy of another, test this twice to make sure that we don't re-introduce #14304
        assert_raises_rpc_error(-1, "BerkeleyBatch: Can't open database w8_copy (duplicates fileid", self.nodes[0].loadwallet, 'w8_copy')


        # Fail to load if wallet file is a symlink
        assert_raises_rpc_error(-4, "Wallet file verification failed: Invalid -wallet path 'w8_symlink'", self.nodes[0].loadwallet, 'w8_symlink')

        self.log.info("Test dynamic wallet creation.")

        # Fail to create a wallet if it already exists.
        assert_raises_rpc_error(-4, "Wallet w2 already exists.", self.nodes[0].createwallet, 'w2')

        # Successfully create a wallet with a new name
        loadwallet_name = self.nodes[0].createwallet('w9')
        assert_equal(loadwallet_name['name'], 'w9')
        w9 = node.get_wallet_rpc('w9')
        assert_equal(w9.getwalletinfo()['walletname'], 'w9')

        assert 'w9' in self.nodes[0].listwallets()

        # Successfully create a wallet using a full path
        new_wallet_dir = os.path.join(self.options.tmpdir, 'new_walletdir')
        new_wallet_name = os.path.join(new_wallet_dir, 'w10')
        loadwallet_name = self.nodes[0].createwallet(new_wallet_name)
        assert_equal(loadwallet_name['name'], new_wallet_name)
        w10 = node.get_wallet_rpc(new_wallet_name)
        assert_equal(w10.getwalletinfo()['walletname'], new_wallet_name)

        assert new_wallet_name in self.nodes[0].listwallets()

        # Fail to load if a directory is specified that doesn't contain a wallet
        os.mkdir(wallet_dir('empty_wallet_dir'))
        assert_raises_rpc_error(-18, "Directory empty_wallet_dir does not contain a wallet.dat file", self.nodes[0].loadwallet, 'empty_wallet_dir')

        self.log.info("Test dynamic wallet unloading")

        # Test `unloadwallet` errors
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[0].unloadwallet)
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded", self.nodes[0].unloadwallet, "dummy")
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded", node.get_wallet_rpc("dummy").unloadwallet)
        assert_raises_rpc_error(-8, "Cannot unload the requested wallet", w1.unloadwallet, "w2"),

        # Successfully unload the specified wallet name
        self.nodes[0].unloadwallet("w1")
        assert 'w1' not in self.nodes[0].listwallets()

        # Successfully unload the wallet referenced by the request endpoint
        # Also ensure unload works during walletpassphrase timeout
        w2.encryptwallet('test')
        w2.walletpassphrase('test', 1)
        w2.unloadwallet()
        time.sleep(1.1)
        assert 'w2' not in self.nodes[0].listwallets()

        # Successfully unload all wallets
        for wallet_name in self.nodes[0].listwallets():
            self.nodes[0].unloadwallet(wallet_name)
        assert_equal(self.nodes[0].listwallets(), [])
        assert_raises_rpc_error(-32601, "Method not found (wallet method is disabled because no wallet is loaded)", self.nodes[0].getwalletinfo)

        # Successfully load a previously unloaded wallet
        self.nodes[0].loadwallet('w1')
        assert_equal(self.nodes[0].listwallets(), ['w1'])
        assert_equal(w1.getwalletinfo()['walletname'], 'w1')

if __name__ == '__main__':
    MultiWalletTest().main()
