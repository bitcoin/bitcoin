#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpful routines for mempool testing."""
from decimal import Decimal

from .util import (
    assert_equal,
    assert_greater_than,
    create_lots_of_big_transactions,
    gen_return_txouts,
)


def fill_mempool(test_framework, node, miniwallet):
    """Fill mempool until eviction.

    Allows for simpler testing of scenarios with floating mempoolminfee > minrelay
    Requires -datacarriersize=100000 and
   -maxmempool=5.
    It will not ensure mempools become synced as it
    is based on a single node and assumes -minrelaytxfee
    is 1 sat/vbyte.
    To avoid unintentional tx dependencies, it is recommended to use separate miniwallets for
    mempool filling vs transactions in tests.
    """
    test_framework.log.info("Fill the mempool until eviction is triggered and the mempoolminfee rises")
    txouts = gen_return_txouts()
    relayfee = node.getnetworkinfo()['relayfee']

    assert_equal(relayfee, Decimal('0.00001000'))

    tx_batch_size = 1
    num_of_batches = 75
    # Generate UTXOs to flood the mempool
    # 1 to create a tx initially that will be evicted from the mempool later
    # 75 transactions each with a fee rate higher than the previous one
    test_framework.generate(miniwallet, 1 + (num_of_batches * tx_batch_size))

    # Mine COINBASE_MATURITY - 1 blocks so that the UTXOs are allowed to be spent
    test_framework.generate(node, 100 - 1)

    # Get all UTXOs up front to ensure none of the transactions spend from each other, as that may
    # change their effective feerate and thus the order in which they are selected for eviction.
    confirmed_utxos = [miniwallet.get_utxo(confirmed_only=True) for _ in range(num_of_batches * tx_batch_size + 1)]
    assert_equal(len(confirmed_utxos), num_of_batches * tx_batch_size + 1)

    test_framework.log.debug("Create a mempool tx that will be evicted")
    tx_to_be_evicted_id = miniwallet.send_self_transfer(from_node=node, utxo_to_spend=confirmed_utxos[0], fee_rate=relayfee)["txid"]
    del confirmed_utxos[0]

    # Increase the tx fee rate to give the subsequent transactions a higher priority in the mempool
    # The tx has an approx. vsize of 65k, i.e. multiplying the previous fee rate (in sats/kvB)
    # by 130 should result in a fee that corresponds to 2x of that fee rate
    base_fee = relayfee * 130

    test_framework.log.debug("Fill up the mempool with txs with higher fee rate")
    with node.assert_debug_log(["rolling minimum fee bumped"]):
        for batch_of_txid in range(num_of_batches):
            fee = (batch_of_txid + 1) * base_fee
            utxos = confirmed_utxos[:tx_batch_size]
            create_lots_of_big_transactions(miniwallet, node, fee, tx_batch_size, txouts, utxos)
            del confirmed_utxos[:tx_batch_size]

    test_framework.log.debug("The tx should be evicted by now")
    # The number of transactions created should be greater than the ones present in the mempool
    assert_greater_than(tx_batch_size * num_of_batches, len(node.getrawmempool()))
    # Initial tx created should not be present in the mempool anymore as it had a lower fee rate
    assert tx_to_be_evicted_id not in node.getrawmempool()

    test_framework.log.debug("Check that mempoolminfee is larger than minrelaytxfee")
    assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
    assert_greater_than(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))
