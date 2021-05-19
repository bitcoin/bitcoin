#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool descendants/ancestors information update.

Test mempool update of transaction descendants/ancestors information (count, size)
when transactions have been re-added from a disconnected block to the mempool.
"""
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MempoolUpdateFromBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-limitdescendantsize=1000', '-limitancestorsize=1000']]

    def transaction_graph_test(self, size, n_tx_to_mine):
        """Create an acyclic tournament (a type of directed graph) of transactions and use it for testing.

        Keyword arguments:
        size -- the order N of the tournament which is equal to the number of the created transactions
        n_tx_to_mine -- the number of transaction that should be mined into a block

        If all of the N created transactions tx[0]..tx[N-1] reside in the mempool,
        the following holds:
            the tx[K] transaction:
            - has N-K descendants (including this one), and
            - has K+1 ancestors (including this one)

        More details: https://en.wikipedia.org/wiki/Tournament_(graph_theory)

        Note: In order to maintain Miniwallet compatibility wallet.send_self_transfer()
        only explicitly sets one ancestor for each transaction to form something
        like a singly linked list of size number of transactions.
        The acyclic tournament is then formed by the mempool.
        """

        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.log.info('Creating {} transactions...'.format(size))
        self.wallet.scan_blocks(start=76, num=1)
        node.generate(size)
        txs = []
        block_ids = []
        tx_size = []
        for i in range(size):
            tx = self.wallet.send_self_transfer(from_node=node)
            m_tx = node.getrawmempool(True)[tx['txid']]
            txs.append(tx)
            tx_size.append(m_tx['vsize'])
            if (i+1) % n_tx_to_mine == 0:
                assert_equal(len(node.getrawmempool()), n_tx_to_mine)
                self.log.info('The batch of {} transactions has been accepted into the mempool.'
                              .format(n_tx_to_mine))
                block_ids.append(self.wallet.generate(1)[0])
                assert_equal(len(node.getrawmempool()), 0)
                self.log.info(
                    'All of the transactions from the current batch have been mined into a block.')

        self.log.info(
            'Mempool size pre-invalidation: {}'.format(len(node.getrawmempool())))
        assert_equal(len(node.getrawmempool()), 0)
        # Invalidate the first block to send the transactions back to the mempool.
        start = time.time()
        node.invalidateblock(block_ids[0])
        end = time.time()
        self.log.info(
            'Mempool size post-invalidation: {}'.format(len(node.getrawmempool())))
        assert_equal(len(node.getrawmempool()), size)
        self.log.info('All of the recently mined transactions have been re-added into the mempool in {} seconds.'
                      .format(end - start))

        self.log.info(
            'Checking descendants/ancestors properties of all of the in-mempool transactions...')
        for k, tx in enumerate(txs):
            id = tx['txid']
            self.log.debug('Check transaction #{}.'.format(k))
            assert_equal(node.getrawmempool(True)[id]['descendantcount'], size - k)
            assert_equal(node.getrawmempool(True)[id]['descendantsize'], sum(tx_size[k:size]))
            assert_equal(node.getrawmempool(True)[id]['ancestorcount'], k + 1)
            assert_equal(node.getrawmempool(True)[id]['ancestorsize'], sum(tx_size[0:(k + 1)]))

    def run_test(self):
        # Use batch size limited by DEFAULT_ANCESTOR_LIMIT = 25
        # to not fire "too many unconfirmed parents" error.
        self.transaction_graph_test(size=100, n_tx_to_mine=25)


if __name__ == '__main__':
    MempoolUpdateFromBlockTest().main()
