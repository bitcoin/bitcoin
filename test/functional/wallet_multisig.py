#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC calls based multisig wallets.

Currently only covers descriptor based based wallets. Test for non-descriptor
based multisig wallets are in wallet_address_types.py and wallet_labels.py and
feature-segwit.py and rpc_psbt.py. Non-wallet multisig tests are in
rpc_createmultisig.py, rpc_fundrawtransaction.py, rpc_rawtransaction.py and rpc_psbt.py.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error
)

class WalletMultisigTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], ["-addresstype=bech32"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        compressed_1 = "0296b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52"
        compressed_2 = "037211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073"

        # w1: blank, watch-only wallet, not descriptor based
        self.nodes[1].createwallet(wallet_name='w1', disable_private_keys=False, blank=True, descriptor=False)
        w1 = self.nodes[1].get_wallet_rpc('w1')

        # w2: blank, watch-only wallet, descriptor based
        self.nodes[1].createwallet(wallet_name='w2', disable_private_keys=True, blank=True, descriptor=True)
        w2 = self.nodes[1].get_wallet_rpc('w2')

        self.log.info("Test addmultisigaddress")
        # TODO: add one private key, because addmultisigaddress is only intended for use with non-watchonly addresses
        w1.addmultisigaddress(2, [compressed_1, compressed_2])


if __name__ == '__main__':
    WalletMultisigTest().main()
