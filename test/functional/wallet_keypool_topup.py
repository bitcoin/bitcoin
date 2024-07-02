#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test HD Wallet keypool restore function.

Two nodes. Node1 is under test. Node0 is providing transactions and generating blocks.

- Start node1, shutdown and backup wallet.
- Generate 110 keys (enough to drain the keypool). Store key 90 (in the initial keypool) and key 110 (beyond the initial keypool). Send funds to key 90 and key 110.
- Stop node1, clear the datadir, move wallet file back into the datadir and restart node1.
- connect node1 to node0. Verify that they sync and node1 receives its funds."""
import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class KeypoolRestoreTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.extra_args = [[]]
        for _ in range(self.num_nodes - 1):
            self.extra_args.append(['-keypool=100'])

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        wallet_path = self.nodes[1].wallets_path / self.default_wallet_name / self.wallet_data_filename
        wallet_backup_path = self.nodes[1].datadir_path / "wallet.bak"
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)

        self.log.info("Make backup of wallet")
        self.stop_node(1)
        shutil.copyfile(wallet_path, wallet_backup_path)
        self.start_node(1, self.extra_args[1])
        for i in [1, 2, 3, 4]:
            self.connect_nodes(0, i)

        output_types = ["legacy", "p2sh-segwit", "bech32"]
        if self.options.descriptors:
            output_types.append("bech32m")
        for i, output_type in enumerate(output_types):
            self.log.info("Generate keys for wallet with address type: {}".format(output_type))
            idx = i+1
            for _ in range(90):
                addr_oldpool = self.nodes[idx].getnewaddress(address_type=output_type)
            for _ in range(20):
                addr_extpool = self.nodes[idx].getnewaddress(address_type=output_type)

            # Make sure we're creating the outputs we expect
            address_details = self.nodes[idx].validateaddress(addr_extpool)
            if i == 0:
                assert not address_details["isscript"] and not address_details["iswitness"]
            elif i == 1:
                assert address_details["isscript"] and not address_details["iswitness"]
            elif i == 2:
                assert not address_details["isscript"] and address_details["iswitness"]
            elif i == 3:
                assert address_details["isscript"] and address_details["iswitness"]

            self.log.info("Send funds to wallet")
            self.nodes[0].sendtoaddress(addr_oldpool, 10)
            self.generate(self.nodes[0], 1)
            self.nodes[0].sendtoaddress(addr_extpool, 5)
            self.generate(self.nodes[0], 1)

            self.log.info("Restart node with wallet backup")
            self.stop_node(idx)
            shutil.copyfile(wallet_backup_path, wallet_path)
            self.start_node(idx, self.extra_args[idx])
            self.connect_nodes(0, idx)
            self.sync_all()

            self.log.info("Verify keypool is restored and balance is correct")
            assert_equal(self.nodes[idx].getbalance(), 15)
            assert_equal(self.nodes[idx].listtransactions()[0]['category'], "receive")
            # Check that we have marked all keys up to the used keypool key as used
            if self.options.descriptors:
                if output_type == 'legacy':
                    assert_equal(self.nodes[idx].getaddressinfo(self.nodes[idx].getnewaddress(address_type=output_type))['hdkeypath'], "m/44h/1h/0h/0/110")
                elif output_type == 'p2sh-segwit':
                    assert_equal(self.nodes[idx].getaddressinfo(self.nodes[idx].getnewaddress(address_type=output_type))['hdkeypath'], "m/49h/1h/0h/0/110")
                elif output_type == 'bech32':
                    assert_equal(self.nodes[idx].getaddressinfo(self.nodes[idx].getnewaddress(address_type=output_type))['hdkeypath'], "m/84h/1h/0h/0/110")
                elif output_type == 'bech32m':
                    assert_equal(self.nodes[idx].getaddressinfo(self.nodes[idx].getnewaddress(address_type=output_type))['hdkeypath'], "m/86h/1h/0h/0/110")
            else:
                assert_equal(self.nodes[idx].getaddressinfo(self.nodes[idx].getnewaddress(address_type=output_type))['hdkeypath'], "m/0'/0'/110'")


if __name__ == '__main__':
    KeypoolRestoreTest(__file__).main()
