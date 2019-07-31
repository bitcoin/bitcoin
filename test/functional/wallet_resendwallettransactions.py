#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the wallet resends transactions periodically."""
from collections import defaultdict
import time

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import ToHex
from test_framework.mininode import P2PInterface, mininode_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, wait_until

class P2PStoreTxInvs(P2PInterface):
    def __init__(self):
        super().__init__()
        self.tx_invs_received = defaultdict(int)

    def on_inv(self, message):
        # Store how many times invs have been received for each tx.
        for i in message.inv:
            if i.type == 1:
                # save txid
                self.tx_invs_received[i.hash] += 1

class ResendWalletTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]  # alias

        node.add_p2p_connection(P2PStoreTxInvs())

        self.log.info("Create a new transaction and wait until it's broadcast")
        txid = int(node.sendtoaddress(node.getnewaddress(), 1), 16)

        # Can take a few seconds due to transaction trickling
        wait_until(lambda: node.p2p.tx_invs_received[txid] >= 1, lock=mininode_lock)

        # Add a second peer since txs aren't rebroadcast to the same peer (see filterInventoryKnown)
        node.add_p2p_connection(P2PStoreTxInvs())

        self.log.info("Create a block")
        # Create and submit a block without the transaction.
        # Transactions are only rebroadcast if there has been a block at least five minutes
        # after the last time we tried to broadcast. Use mocktime and give an extra minute to be sure.
        block_time = int(time.time()) + 6 * 60
        node.setmocktime(block_time)
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(node.getblockchaininfo()['blocks']), block_time)
        block.nVersion = 3
        block.rehash()
        block.solve()
        node.submitblock(ToHex(block))

        # Transaction should not be rebroadcast
        node.p2ps[1].sync_with_ping()
        assert_equal(node.p2ps[1].tx_invs_received[txid], 0)

        self.log.info("Transaction should be rebroadcast after 30 minutes")
        # Use mocktime and give an extra 5 minutes to be sure.
        rebroadcast_time = int(time.time()) + 41 * 60
        node.setmocktime(rebroadcast_time)
        wait_until(lambda: node.p2ps[1].tx_invs_received[txid] >= 1, lock=mininode_lock)

if __name__ == '__main__':
    ResendWalletTransactionsTest().main()
