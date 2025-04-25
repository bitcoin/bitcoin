#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class WalletHDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ['-keypool=0']]
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Make sure we use hd, keep masterkeyid
        hd_fingerprint = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['hdmasterfingerprint']
        assert_equal(len(hd_fingerprint), 8)

        # create an internal key
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV = self.nodes[1].getaddressinfo(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/84h/1h/0h/1/0")

        # Import a non-HD private key in the HD wallet
        non_hd_add = 'bcrt1qmevj8zfx0wdvp05cqwkmr6mxkfx60yezwjksmt'
        non_hd_key = 'cS9umN9w6cDMuRVYdbkfE4c7YUFLJRoXMfhQ569uY4odiQbVN8Rt'
        self.nodes[1].importprivkey(non_hd_key)

        # This should be enough to keep the master key and the non-HD key
        self.nodes[1].backupwallet(self.nodes[1].datadir_path / "hd.bak")
        #self.nodes[1].dumpwallet(self.nodes[1].datadir_path / "hd.dump")

        # Derive some HD addresses and remember the last
        # Also send funds to each add
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)
        hd_add = None
        NUM_HD_ADDS = 10
        for i in range(1, NUM_HD_ADDS + 1):
            hd_add = self.nodes[1].getnewaddress()
            hd_info = self.nodes[1].getaddressinfo(hd_add)
            assert_equal(hd_info["hdkeypath"], "m/84h/1h/0h/0/" + str(i))
            assert_equal(hd_info["hdmasterfingerprint"], hd_fingerprint)
            self.nodes[0].sendtoaddress(hd_add, 1)
            self.generate(self.nodes[0], 1)
        self.nodes[0].sendtoaddress(non_hd_add, 1)
        self.generate(self.nodes[0], 1)

        # create an internal key (again)
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV = self.nodes[1].getaddressinfo(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/84h/1h/0h/1/1")

        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        self.log.info("Restore backup ...")
        self.stop_node(1)
        # we need to delete the complete chain directory
        # otherwise node1 would auto-recover all funds in flag the keypool keys as used
        shutil.rmtree(self.nodes[1].blocks_path)
        shutil.rmtree(self.nodes[1].chain_path / "chainstate")
        shutil.copyfile(
            self.nodes[1].datadir_path / "hd.bak",
            self.nodes[1].wallets_path / self.default_wallet_name / self.wallet_data_filename
        )
        self.start_node(1)

        # Assert that derivation is deterministic
        hd_add_2 = None
        for i in range(1, NUM_HD_ADDS + 1):
            hd_add_2 = self.nodes[1].getnewaddress()
            hd_info_2 = self.nodes[1].getaddressinfo(hd_add_2)
            assert_equal(hd_info_2["hdkeypath"], "m/84h/1h/0h/0/" + str(i))
            assert_equal(hd_info_2["hdmasterfingerprint"], hd_fingerprint)
        assert_equal(hd_add, hd_add_2)
        self.connect_nodes(0, 1)
        self.sync_all()

        # Needs rescan
        self.nodes[1].rescanblockchain()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # Try a RPC based rescan
        self.stop_node(1)
        shutil.rmtree(self.nodes[1].blocks_path)
        shutil.rmtree(self.nodes[1].chain_path / "chainstate")
        shutil.copyfile(
            self.nodes[1].datadir_path / "hd.bak",
            self.nodes[1].wallets_path / self.default_wallet_name / self.wallet_data_filename
        )
        self.start_node(1, extra_args=self.extra_args[1])
        self.connect_nodes(0, 1)
        self.sync_all()
        # Wallet automatically scans blocks older than key on startup
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)
        out = self.nodes[1].rescanblockchain(0, 1)
        assert_equal(out['start_height'], 0)
        assert_equal(out['stop_height'], 1)
        out = self.nodes[1].rescanblockchain()
        assert_equal(out['start_height'], 0)
        assert_equal(out['stop_height'], self.nodes[1].getblockcount())
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # send a tx and make sure its using the internal chain for the changeoutput
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        outs = self.nodes[1].gettransaction(txid=txid, verbose=True)['decoded']['vout']
        keypath = ""
        for out in outs:
            if out['value'] != 1:
                keypath = self.nodes[1].getaddressinfo(out['scriptPubKey']['address'])['hdkeypath']

        assert_equal(keypath[0:14], "m/84h/1h/0h/1/")


if __name__ == '__main__':
    WalletHDTest(__file__).main()
