#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool descendants/ancestors information update.

Test mempool update of transaction descendants/ancestors information (count, size)
when transactions have been re-added from a disconnected block to the mempool.
"""
from decimal import Decimal
from math import ceil
import time

from test_framework.blocktools import create_empty_fork
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet
from test_framework.mempool_util import DEFAULT_CLUSTER_LIMIT

MAX_DISCONNECTED_TX_POOL_BYTES = 20_000_000

CUSTOM_ANCESTOR_COUNT = 100
CUSTOM_DESCENDANT_COUNT = CUSTOM_ANCESTOR_COUNT

class MempoolUpdateFromBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-limitclustersize=1000']]

    def trigger_reorg(self, fork_blocks):
        """Trigger reorg of the fork blocks."""
        for block in fork_blocks:
            self.nodes[0].submitblock(block.serialize().hex())
        assert_equal(self.nodes[0].getbestblockhash(), fork_blocks[-1].hash_hex)

    def transaction_graph_test(self, size, *, n_tx_to_mine, fee=100_000):
        """Create an acyclic tournament (a type of directed graph) of transactions and use it for testing.

        Keyword arguments:
        size -- the order N of the tournament which is equal to the number of the created transactions
        n_tx_to_mine -- the number of transactions that should be mined into a block

        If all of the N created transactions tx[0]..tx[N-1] reside in the mempool,
        the following holds:
            the tx[K] transaction:
            - has N-K descendants (including this one), and
            - has K+1 ancestors (including this one)

        More details: https://en.wikipedia.org/wiki/Tournament_(graph_theory)
        """
        wallet = MiniWallet(self.nodes[0])

        # Prep for fork with empty blocks to not use invalidateblock directly
        # for reorg case. The rpc has different codepath
        fork_blocks = create_empty_fork(self.nodes[0], fork_length=7)

        tx_id = []
        tx_size = []
        self.log.info('Creating {} transactions...'.format(size))
        for i in range(0, size):
            self.log.debug('Preparing transaction #{}...'.format(i))
            # Prepare inputs.
            if i == 0:
                inputs = [wallet.get_utxo()]  # let MiniWallet provide a start UTXO
            else:
                inputs = []
                for j, tx in enumerate(tx_id[0:i]):
                    # Transaction tx[K] is a child of each of previous transactions tx[0]..tx[K-1] at their output K-1.
                    vout = i - j - 1
                    inputs.append(wallet.get_utxo(txid=tx_id[j], vout=vout))

            # Prepare outputs.
            tx_count = i + 1
            if tx_count < size:
                # Transaction tx[K] is an ancestor of each of subsequent transactions tx[K+1]..tx[N-1].
                n_outputs = size - tx_count
            else:
                n_outputs = 1

            # Create a new transaction.
            new_tx = wallet.send_self_transfer_multi(
                from_node=self.nodes[0],
                utxos_to_spend=inputs,
                num_outputs=n_outputs,
                fee_per_output=ceil(fee / n_outputs)
            )
            tx_id.append(new_tx['txid'])
            tx_size.append(new_tx['tx'].get_vsize())

            if tx_count in n_tx_to_mine:
                # The created transactions are mined into blocks by batches.
                self.log.info('The batch of {} transactions has been accepted into the mempool.'.format(len(self.nodes[0].getrawmempool())))
                self.generate(self.nodes[0], 1)[0]
                assert_equal(len(self.nodes[0].getrawmempool()), 0)
                self.log.info('All of the transactions from the current batch have been mined into a block.')
            elif tx_count == size:
                # At the end the old fork is submitted to cause reorg, and all of the created
                # transactions should be re-added from disconnected blocks to the mempool.
                self.log.info('The last batch of {} transactions has been accepted into the mempool.'.format(len(self.nodes[0].getrawmempool())))
                start = time.time()
                # Trigger reorg
                for block in fork_blocks:
                    self.nodes[0].submitblock(block.serialize().hex())
                end = time.time()
                assert_equal(len(self.nodes[0].getrawmempool()), size)
                self.log.info('All of the recently mined transactions have been re-added into the mempool in {} seconds.'.format(end - start))

        self.log.info('Checking descendants/ancestors properties of all of the in-mempool transactions...')
        for k, tx in enumerate(tx_id):
            self.log.debug('Check transaction #{}.'.format(k))
            entry = self.nodes[0].getmempoolentry(tx)
            assert_equal(entry['descendantcount'], size - k)
            assert_equal(entry['descendantsize'], sum(tx_size[k:size]))
            assert_equal(entry['ancestorcount'], k + 1)
            assert_equal(entry['ancestorsize'], sum(tx_size[0:(k + 1)]))

        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        wallet.rescan_utxos()

    def test_max_disconnect_pool_bytes(self):
        self.log.info('Creating independent transactions to test MAX_DISCONNECTED_TX_POOL_BYTES limit during reorg')

        # Generate coins for the hundreds of transactions we will make
        parent_target_vsize = 100_000
        wallet = MiniWallet(self.nodes[0])
        self.generate(wallet, (MAX_DISCONNECTED_TX_POOL_BYTES // parent_target_vsize) + 100)

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Set up empty fork blocks ahead of time, needs to be longer than full fork made later
        fork_blocks = create_empty_fork(self.nodes[0], fork_length=60)

        large_std_txs = []
        # Add children to ensure they're recursively removed if disconnectpool trimming of parent occurs
        small_child_txs = []
        aggregate_serialized_size = 0
        while aggregate_serialized_size < MAX_DISCONNECTED_TX_POOL_BYTES:
            # Mine parents in FIFO order via fee ordering
            large_std_txs.append(wallet.create_self_transfer(target_vsize=parent_target_vsize, fee=Decimal("0.00400000") - (Decimal("0.00001000") * len(large_std_txs))))
            small_child_txs.append(wallet.create_self_transfer(utxo_to_spend=large_std_txs[-1]['new_utxo']))
            # Slight underestimate of dynamic cost, so we'll be over during reorg
            aggregate_serialized_size += len(large_std_txs[-1]["tx"].serialize())

        for large_std_tx in large_std_txs:
            self.nodes[0].sendrawtransaction(large_std_tx["hex"])

        assert_equal(self.nodes[0].getmempoolinfo()["size"], len(large_std_txs))

        # Mine non-empty chain that will be reorged shortly
        self.generate(self.nodes[0], len(fork_blocks) - 1)
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Stick children in mempool, evicted with parent potentially
        for small_child_tx in small_child_txs:
            self.nodes[0].sendrawtransaction(small_child_tx["hex"])

        assert_equal(self.nodes[0].getmempoolinfo()["size"], len(small_child_txs))

        # Reorg back before the first block in the series, should drop something
        # but not all, and any time parent is dropped, child is also removed
        self.trigger_reorg(fork_blocks=fork_blocks)
        mempool = self.nodes[0].getrawmempool()
        expected_parent_count = len(large_std_txs) - 2
        assert_equal(len(mempool), expected_parent_count * 2)

        # The txns at the end of the list, or most recently confirmed, should have been trimmed
        assert_equal([tx["txid"] in mempool for tx in large_std_txs], [tx["txid"] in mempool for tx in small_child_txs])
        assert_equal([tx["txid"] in mempool for tx in large_std_txs], [True] * expected_parent_count + [False] * 2)

    def test_chainlimits_exceeded(self):
        self.log.info('Check that too long chains on reorg are handled')

        wallet = MiniWallet(self.nodes[0])
        self.generate(wallet, 101)

        assert_equal(self.nodes[0].getrawmempool(), [])

        # Prep fork
        fork_blocks = create_empty_fork(self.nodes[0])

        # Two higher than descendant count
        chain = wallet.create_self_transfer_chain(chain_length=DEFAULT_CLUSTER_LIMIT + 2)
        for tx in chain[:-2]:
            self.nodes[0].sendrawtransaction(tx["hex"])

        assert_raises_rpc_error(-26, "too-large-cluster", self.nodes[0].sendrawtransaction, chain[-2]["hex"])

        # Mine a block with all but last transaction, non-standardly long chain
        self.generateblock(self.nodes[0], output="raw(42)", transactions=[tx["hex"] for tx in chain[:-1]])
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Last tx fits now
        self.nodes[0].sendrawtransaction(chain[-1]["hex"])

        # Finally, reorg to empty chain to kick everything back into mempool
        # at normal chain limits
        for block in fork_blocks:
            self.nodes[0].submitblock(block.serialize().hex())
        mempool = self.nodes[0].getrawmempool()
        assert_equal(set(mempool), set([tx["txid"] for tx in chain[:-2]]))

    def run_test(self):
        # Mine in batches of 25 to test multi-block reorg under chain limits
        self.transaction_graph_test(size=DEFAULT_CLUSTER_LIMIT, n_tx_to_mine=[25, 50, 75])

        self.test_max_disconnect_pool_bytes()

        self.test_chainlimits_exceeded()

if __name__ == '__main__':
    MempoolUpdateFromBlockTest(__file__).main()
