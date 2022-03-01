#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Backwards compatibility MWEB wallet test

Test various backwards compatibility scenarios. Download the previous node binaries:

test/get_previous_releases.py -b v0.18.1 v0.17.1 v0.16.3 v0.15.1

v0.15.2 is not required by this test, but it is used in wallet_upgradewallet.py.
Due to a hardfork in regtest, it can't be used to sync nodes.


Due to RPC changes introduced in various versions the below tests
won't work for older versions without some patches or workarounds.

Use only the latest patch version of each release, unless a test specifically
needs an older patch version.
"""

import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create

from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class MWEBWalletAddressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        # Add new version after each release:
        self.extra_args = [
            ["-addresstype=bech32", "-keypool=2"], # current wallet version
            ["-usehd=0"], # v0.15.1
        ]
        self.wallet_names = [self.default_wallet_name, None]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.setup_nodes()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            150100,
        ])

        self.start_nodes()
        self.import_deterministic_coinbase_privkeys()
        
    def run_test(self):
        self.test_prehd_wallet()
        # TODO: self.test_blank_wallet()
        # TODO: self.test_keys_disabled()

    def test_prehd_wallet(self):
        self.nodes[0].generatetoaddress(101, self.nodes[0].getnewaddress())

        node_master = self.nodes[0]
        node_master_wallet_dir = os.path.join(node_master.datadir, "regtest/wallets", self.default_wallet_name)
        node_master_wallet = os.path.join(node_master_wallet_dir, self.default_wallet_name, self.wallet_data_filename)

        v15_1_node  = self.nodes[1]
        v15_1_wallet = os.path.join(v15_1_node.datadir, "regtest/wallet.dat")
        self.stop_node(1)
        
        # Copy the 0.15.1 non hd wallet to the last Litecoin Core version and open it:
        node_master.get_wallet_rpc(self.default_wallet_name).unloadwallet()
        shutil.rmtree(node_master_wallet_dir)
        os.mkdir(node_master_wallet_dir)
        shutil.copy(
            v15_1_wallet,
            node_master_wallet_dir
        )
        node_master.loadwallet(self.default_wallet_name)
        
        # MW: TODO - Need a more appropriate error message
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", node_master.getnewaddress, address_type='mweb')

if __name__ == '__main__':
    MWEBWalletAddressTest().main()
