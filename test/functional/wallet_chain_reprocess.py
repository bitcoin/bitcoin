#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    assert_greater_than,
)

class WalletChainReprocess(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_process_all_blocks_after_unclean(self):
        self.log.info("Testing that all blocks are reprocessed by the wallet after an unclean shutdown")
        self.nodes[0].createwallet("target")
        wallet = self.nodes[0].get_wallet_rpc("target")

        # Mine 1 block to the wallet, 100 to somewhere else and flush the chainstate with a restart
        self.generatetoaddress(self.nodes[0], 1, wallet.getnewaddress(), sync_fun=self.no_op)
        self.generate(self.nodes[0], 101, sync_fun=self.no_op)
        self.restart_node(0)
        self.connect_nodes(0, 1)
        self.nodes[0].loadwallet("target")
        wallet = self.nodes[0].get_wallet_rpc("target")

        # In order to ensure that wallet transaction states match the chain, we want to mine
        # enough blocks such that the chainstate will still be moving forward while the
        # wallet is loading. 100 blocks should be sufficient.
        # A transaction is included in each block that is a child of a transaction in the
        # previous block so that every block has a wallet transaction.
        txids = []
        for _ in range(0, 100):
            res = wallet.sendall([wallet.getnewaddress()])
            assert_equal(res["complete"], True)
            txids.append(res["txid"])
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        # Sync nodes, kill node 0, and restart
        self.sync_all()
        tip_height = self.nodes[0].getblockchaininfo()["blocks"]
        wallet_height = wallet.getwalletinfo()["lastprocessedblock"]["height"]
        assert_equal(tip_height, wallet_height)
        wallet.unloadwallet()
        self.nodes[0].kill_process()
        self.start_node(0)
        restart_tip_height = self.nodes[0].getblockchaininfo()["blocks"]
        assert_greater_than(tip_height, restart_tip_height)
        assert_greater_than(wallet_height, restart_tip_height)

        self.nodes[0].loadwallet("target")
        wallet = self.nodes[0].get_wallet_rpc("target")
        assert_greater_than(wallet_height, wallet.getwalletinfo()["lastprocessedblock"]["height"])

        # Wait for node0 to be synced
        self.connect_nodes(0, 1)
        self.sync_all()
        assert_equal(wallet.getwalletinfo()["lastprocessedblock"]["height"], self.nodes[0].getblockcount())

        # Check every transaction is confirmed
        for txid in txids:
            tx = wallet.gettransaction(txid)
            assert_greater_than(tx["confirmations"], 0)

            # None of these transactions can be abandoned either
            assert_raises_rpc_error(-5, "Transaction not eligible for abandonment", wallet.abandontransaction, txid)


    def run_test(self):
        self.test_process_all_blocks_after_unclean()

if __name__ == '__main__':
    WalletChainReprocess(__file__).main()
