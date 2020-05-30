#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the wallet resends transactions periodically."""
import time

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import ToHex
from test_framework.mininode import P2PTxInvStore, mininode_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, wait_until

class ResendWalletTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]  # alias

        node.add_p2p_connection(P2PTxInvStore())

        self.log.info("Create a new transaction and wait until it's broadcast")
        txid = int(node.sendtoaddress(node.getnewaddress(), 1), 16)

        # Wallet rebroadcast is first scheduled 1 sec after startup (see
        # nNextResend in ResendWalletTransactions()). Sleep for just over a
        # second to be certain that it has been called before the first
        # setmocktime call below.
        time.sleep(1.1)

        # Can take a few seconds due to transaction trickling
        wait_until(lambda: node.p2p.tx_invs_received[txid] >= 1, lock=mininode_lock)

        # Add a second peer since txs aren't rebroadcast to the same peer (see filterInventoryKnown)
        node.add_p2p_connection(P2PTxInvStore())

        self.log.info("Create a block")
        # Create and submit a block without the transaction.
        # Transactions are only rebroadcast if there has been a block at least five minutes
        # after the last time we tried to broadcast. Use mocktime and give an extra minute to be sure.
        block_time = int(time.time()) + 6 * 60
        node.setmocktime(block_time)
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1), block_time)
        block.rehash()
        block.solve()
        node.submitblock(ToHex(block))

        node.syncwithvalidationinterfacequeue()
        now = int(time.time())

        # Transaction should not be rebroadcast within first 12 hours
        # Leave 2 mins for buffer
        twelve_hrs = 12 * 60 * 60
        two_min = 2 * 60
        node.setmocktime(now + twelve_hrs - two_min)
        time.sleep(2) # ensure enough time has passed for rebroadcast attempt to occur
        assert_equal(txid in node.p2ps[1].get_invs(), False)

        self.log.info("Bump time & check that transaction is rebroadcast")
        # Transaction should be rebroadcast approximately 24 hours in the future,
        # but can range from 12-36. So bump 36 hours to be sure.
        node.setmocktime(now + 36 * 60 * 60)
        wait_until(lambda: node.p2ps[1].tx_invs_received[txid] >= 1, lock=mininode_lock)


if __name__ == '__main__':
    ResendWalletTransactionsTest().main()
