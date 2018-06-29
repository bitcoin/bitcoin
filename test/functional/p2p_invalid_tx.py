#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid transactions.

In this test we connect to one node over p2p, and test tx requests."""
from test_framework.blocktools import create_block, create_coinbase, create_transaction
from test_framework.messages import COIN
from test_framework.mininode import network_thread_start, P2PDataStore
from test_framework.test_framework import BitcoinTestFramework

class InvalidTxRequestTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-whitelist=127.0.0.1"]]

    def run_test(self):
        # Add p2p connection to node0
        node = self.nodes[0]  # convenience reference to the node
        node.add_p2p_connection(P2PDataStore())

        network_thread_start()
        node.p2p.wait_for_verack()

        best_block = self.nodes[0].getbestblockhash()
        tip = int(best_block, 16)
        best_block_time = self.nodes[0].getblock(best_block)['time']
        block_time = best_block_time + 1

        self.log.info("Create a new block with an anyone-can-spend coinbase.")
        height = 1
        block = create_block(tip, create_coinbase(height), block_time)
        block.solve()
        # Save the coinbase for later
        block1 = block
        tip = block.sha256
        node.p2p.send_blocks_and_test([block], node, success=True)

        self.log.info("Mature the block.")
        self.nodes[0].generate(100)

        # b'\x64' is OP_NOTIF
        # Transaction will be rejected with code 16 (REJECT_INVALID)
        tx1 = create_transaction(block1.vtx[0], 0, b'\x64', 50 * COIN - 12000)
        node.p2p.send_txs_and_test([tx1], node, success=False, reject_code=16, reject_reason=b'mandatory-script-verify-flag-failed (Invalid OP_IF construction)')

        # Verify valid transaction
        tx1 = create_transaction(block1.vtx[0], 0, b'', 50 * COIN - 12000)
        node.p2p.send_txs_and_test([tx1], node, success=True)


if __name__ == '__main__':
    InvalidTxRequestTest().main()
