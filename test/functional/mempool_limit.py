#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool limiting together/eviction with the wallet."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    create_lots_of_big_transactions,
    gen_return_txouts,
)
from test_framework.wallet import MiniWallet


class MempoolLimitTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]]
        self.supports_cli = False

    def run_test(self):
        txouts = gen_return_txouts()
        node = self.nodes[0]
        miniwallet = MiniWallet(node)
        relayfee = node.getnetworkinfo()['relayfee']

        self.log.info('Check that mempoolminfee is minrelaytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        tx_batch_size = 25
        num_of_batches = 3
        # Generate UTXOs to flood the mempool
        # 1 to create a tx initially that will be evicted from the mempool later
        # 3 batches of multiple transactions with a fee rate much higher than the previous UTXO
        # And 1 more to verify that this tx does not get added to the mempool with a fee rate less than the mempoolminfee
        self.generate(miniwallet, 1 + (num_of_batches * tx_batch_size) + 1)

        # Mine 99 blocks so that the UTXOs are allowed to be spent
        self.generate(node, COINBASE_MATURITY - 1)

        self.log.info('Create a mempool tx that will be evicted')
        tx_to_be_evicted_id = miniwallet.send_self_transfer(from_node=node, fee_rate=relayfee)["txid"]

        # Increase the tx fee rate to give the subsequent transactions a higher priority in the mempool
        # The tx has an approx. vsize of 65k, i.e. multiplying the previous fee rate (in sats/kvB)
        # by 130 should result in a fee that corresponds to 2x of that fee rate
        base_fee = relayfee * 130

        self.log.info("Fill up the mempool with txs with higher fee rate")
        for batch_of_txid in range(num_of_batches):
            fee = (batch_of_txid + 1) * base_fee
            create_lots_of_big_transactions(miniwallet, node, fee, tx_batch_size, txouts)

        self.log.info('The tx should be evicted by now')
        # The number of transactions created should be greater than the ones present in the mempool
        assert_greater_than(tx_batch_size * num_of_batches, len(node.getrawmempool()))
        # Initial tx created should not be present in the mempool anymore as it had a lower fee rate
        assert tx_to_be_evicted_id not in node.getrawmempool()

        self.log.info('Check that mempoolminfee is larger than minrelaytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_greater_than(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        # Deliberately try to create a tx with a fee less than the minimum mempool fee to assert that it does not get added to the mempool
        self.log.info('Create a mempool tx that will not pass mempoolminfee')
        assert_raises_rpc_error(-26, "mempool min fee not met", miniwallet.send_self_transfer, from_node=node, fee_rate=relayfee)

        self.log.info('Test passing a value below the minimum (5 MB) to -maxmempool throws an error')
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(["-maxmempool=4"], "Error: -maxmempool must be at least 5 MB")


if __name__ == '__main__':
    MempoolLimitTest().main()
