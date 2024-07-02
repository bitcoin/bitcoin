#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that descriptor wallets rescan mempool transactions properly when importing."""

from test_framework.address import (
    address_to_scriptpubkey,
    ADDRESS_BCRT1_UNSPENDABLE,
)
from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import test_address


class WalletRescanUnconfirmed(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def run_test(self):
        self.log.info("Create wallets and mine initial chain")
        node = self.nodes[0]
        tester_wallet = MiniWallet(node)

        node.createwallet(wallet_name='w0', disable_private_keys=False)
        w0 = node.get_wallet_rpc('w0')

        self.log.info("Create a parent tx and mine it in a block that will later be disconnected")
        parent_address = w0.getnewaddress()
        tx_parent_to_reorg = tester_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(parent_address),
            amount=COIN,
        )
        assert tx_parent_to_reorg["txid"] in node.getrawmempool()
        block_to_reorg = self.generate(tester_wallet, 1)[0]
        assert_equal(len(node.getrawmempool()), 0)
        node.syncwithvalidationinterfacequeue()
        assert_equal(w0.gettransaction(tx_parent_to_reorg["txid"])["confirmations"], 1)

        # Create an unconfirmed child transaction from the parent tx, sending all
        # the funds to an unspendable address. Importantly, no change output is created so the
        # transaction can't be recognized using its outputs. The wallet rescan needs to know the
        # inputs of the transaction to detect it, so the parent must be processed before the child.
        w0_utxos = w0.listunspent()

        self.log.info("Create a child tx and wait for it to propagate to all mempools")
        # The only UTXO available to spend is tx_parent_to_reorg.
        assert_equal(len(w0_utxos), 1)
        assert_equal(w0_utxos[0]["txid"], tx_parent_to_reorg["txid"])
        tx_child_unconfirmed_sweep = w0.sendall([ADDRESS_BCRT1_UNSPENDABLE])
        assert tx_child_unconfirmed_sweep["txid"] in node.getrawmempool()
        node.syncwithvalidationinterfacequeue()

        self.log.info("Mock a reorg, causing parent to re-enter mempools after its child")
        node.invalidateblock(block_to_reorg)
        assert tx_parent_to_reorg["txid"] in node.getrawmempool()

        self.log.info("Import descriptor wallet on another node")
        descriptors_to_import = [{"desc": w0.getaddressinfo(parent_address)['parent_desc'], "timestamp": 0, "label": "w0 import"}]

        node.createwallet(wallet_name="w1", disable_private_keys=True)
        w1 = node.get_wallet_rpc("w1")
        w1.importdescriptors(descriptors_to_import)

        self.log.info("Check that the importing node has properly rescanned mempool transactions")
        # Check that parent address is correctly determined as ismine
        test_address(w1, parent_address, solvable=True, ismine=True)
        # This would raise a JSONRPCError if the transactions were not identified as belonging to the wallet.
        assert_equal(w1.gettransaction(tx_parent_to_reorg["txid"])["confirmations"], 0)
        assert_equal(w1.gettransaction(tx_child_unconfirmed_sweep["txid"])["confirmations"], 0)

if __name__ == '__main__':
    WalletRescanUnconfirmed(__file__).main()
