#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.util import assert_equal
from test_framework.test_framework import BitcoinTestFramework

'''
This test exercises the deprecatedrpc=bip125 flag and deprecated -walletrbf options
and should be removed when the options are removed post deprecation.
'''
class WalletDeprecatedRBFTest(BitcoinTestFramework):
    def set_test_params(self):
      self.num_nodes = 1
      self.setup_clean_chain = True
      self.extra_args = [[]]

    def skip_test_if_missing_module(self):
      self.skip_if_no_wallet()

    def run_test(self):
      self.log.info("Test -deprecatedrpc=bip125 and -walletrbf startup option")

      self.restart_node(0, extra_args=["-walletrbf=0", "-deprecatedrpc=bip125"])
      node = self.nodes[0]
      node.createwallet("deprecated_optinrbf")
      wallet = node.get_wallet_rpc("deprecated_optinrbf")
      self.generatetoaddress(node, nblocks=101, address=wallet.getnewaddress(), sync_fun=self.no_op)
      tx = wallet.gettransaction(wallet.sendall(recipients=[wallet.getnewaddress()])["txid"])
      assert_equal("bip125-replaceable" in tx, True)
      assert_equal(tx["bip125-replaceable"], "no")
      self.stop_node(0, expected_stderr="Warning: -walletrbf is deprecated and will be fully removed in the next release.")


if __name__ == '__main__':
    WalletDeprecatedRBFTest(__file__).main()
