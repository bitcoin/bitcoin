#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpful routines for mempool testing."""
from decimal import Decimal

from .blocktools import (
    COINBASE_MATURITY,
)
from .messages import CTransaction
from .util import (
    assert_equal,
    assert_greater_than,
    create_lots_of_big_transactions,
    gen_return_txouts,
)
from .wallet import (
    MiniWallet,
)

ORPHAN_TX_EXPIRE_TIME = 1200

def assert_mempool_contents(test_framework, node, expected=None, sync=True):
    """Assert that all transactions in expected are in the mempool,
    and no additional ones exist. 'expected' is an array of
    CTransaction objects
    """
    if sync:
        test_framework.sync_mempools()
    if not expected:
        expected = []
    assert_equal(len(expected), len(set(expected)))
    mempool = node.getrawmempool(verbose=False)
    assert_equal(len(mempool), len(expected))
    for tx in expected:
        assert tx.rehash() in mempool


def fill_mempool(test_framework, node, *, tx_sync_fun=None):
    """Fill mempool until eviction.

    Allows for simpler testing of scenarios with floating mempoolminfee > minrelay
    Requires -maxmempool=5 and assumes -minrelaytxfee
    is 1 sat/vbyte.
    To avoid unintentional tx dependencies, the mempool filling txs are created with a
    tagged ephemeral miniwallet instance.
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
    ephemeral_miniwallet = MiniWallet(node, tag_name="fill_mempool_ephemeral_wallet")
    test_framework.generate(ephemeral_miniwallet, 1 + num_of_batches * tx_batch_size)

    # Mine enough blocks so that the UTXOs are allowed to be spent
    test_framework.generate(node, COINBASE_MATURITY - 1)

    # Get all UTXOs up front to ensure none of the transactions spend from each other, as that may
    # change their effective feerate and thus the order in which they are selected for eviction.
    confirmed_utxos = [ephemeral_miniwallet.get_utxo(confirmed_only=True) for _ in range(num_of_batches * tx_batch_size + 1)]
    assert_equal(len(confirmed_utxos), num_of_batches * tx_batch_size + 1)

    test_framework.log.debug("Create a mempool tx that will be evicted")
    tx_to_be_evicted_id = ephemeral_miniwallet.send_self_transfer(
        from_node=node, utxo_to_spend=confirmed_utxos.pop(0), fee_rate=relayfee)["txid"]

    def send_batch(fee):
        utxos = confirmed_utxos[:tx_batch_size]
        create_lots_of_big_transactions(ephemeral_miniwallet, node, fee, tx_batch_size, txouts, utxos)
        del confirmed_utxos[:tx_batch_size]

    # Increase the tx fee rate to give the subsequent transactions a higher priority in the mempool
    # The tx has an approx. vsize of 65k, i.e. multiplying the previous fee rate (in sats/kvB)
    # by 130 should result in a fee that corresponds to 2x of that fee rate
    base_fee = relayfee * 130
    batch_fees = [(i + 1) * base_fee for i in range(num_of_batches)]

    test_framework.log.debug("Fill up the mempool with txs with higher fee rate")
    for fee in batch_fees[:-3]:
        send_batch(fee)
    tx_sync_fun() if tx_sync_fun else test_framework.sync_mempools()  # sync before any eviction
    assert_equal(node.getmempoolinfo()["mempoolminfee"], Decimal("0.00001000"))
    for fee in batch_fees[-3:]:
        send_batch(fee)
    tx_sync_fun() if tx_sync_fun else test_framework.sync_mempools()  # sync after all evictions

    test_framework.log.debug("The tx should be evicted by now")
    # The number of transactions created should be greater than the ones present in the mempool
    assert_greater_than(tx_batch_size * num_of_batches, len(node.getrawmempool()))
    # Initial tx created should not be present in the mempool anymore as it had a lower fee rate
    assert tx_to_be_evicted_id not in node.getrawmempool()

    test_framework.log.debug("Check that mempoolminfee is larger than minrelaytxfee")
    assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
    assert_greater_than(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

def tx_in_orphanage(node, tx: CTransaction) -> bool:
    """Returns true if the transaction is in the orphanage."""
    found = [o for o in node.getorphantxs(verbosity=1) if o["txid"] == tx.rehash() and o["wtxid"] == tx.getwtxid()]
    return len(found) == 1
