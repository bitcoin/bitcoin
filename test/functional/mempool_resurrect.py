#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test resurrection of mined transactions when the blockchain is re-organized."""

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MempoolCoinbaseTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        # Spend block 1/2/3's coinbase transactions
        # Mine a block
        # Create three more transactions, spending the spends
        # Mine another block
        # ... make sure all the transactions are confirmed
        # Invalidate both blocks
        # ... make sure all the transactions are put back in the mempool
        # Mine a new block
        # ... make sure all the transactions are confirmed again
        blocks = []
        spends1_ids = [wallet.send_self_transfer(from_node=node)['txid'] for _ in range(3)]
        blocks.extend(self.generate(node, 1))
        spends2_ids = [wallet.send_self_transfer(from_node=node)['txid'] for _ in range(3)]
        blocks.extend(self.generate(node, 1))

        spends_ids = set(spends1_ids + spends2_ids)

        # mempool should be empty, all txns confirmed
        assert_equal(set(node.getrawmempool()), set())
        confirmed_txns = set(node.getblock(blocks[0])['tx'] + node.getblock(blocks[1])['tx'])
        # Checks that all spend txns are contained in the mined blocks
        assert spends_ids < confirmed_txns

        # Use invalidateblock to re-org back
        node.invalidateblock(blocks[0])

        # All txns should be back in mempool with 0 confirmations
        assert_equal(set(node.getrawmempool()), spends_ids)

        # Generate another block, they should all get mined
        blocks = self.generate(node, 1)
        # mempool should be empty, all txns confirmed
        assert_equal(set(node.getrawmempool()), set())
        confirmed_txns = set(node.getblock(blocks[0])['tx'])
        assert spends_ids < confirmed_txns


if __name__ == '__main__':
    MempoolCoinbaseTest().main()
