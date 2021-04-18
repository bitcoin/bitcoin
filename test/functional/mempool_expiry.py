#!/usr/bin/env python3
# Copyright (c) 2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests that a mempool transaction expires after a given timeout and that its
children are removed as well.

Both the default expiry timeout defined by DEFAULT_MEMPOOL_EXPIRY and a user
definable expiry timeout via the '-mempoolexpiry=<n>' command line argument
(<n> is the timeout in hours) are tested.
"""

from datetime import timedelta

from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)

DEFAULT_MEMPOOL_EXPIRY = 336  # hours
CUSTOM_MEMPOOL_EXPIRY = 10  # hours


class MempoolExpiryTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_transaction_expiry(self, timeout):
        """Tests that a transaction expires after the expiry timeout and its
        children are removed as well."""
        node = self.nodes[0]

        # Send a parent transaction that will expire.
        parent_address = node.getnewaddress()
        parent_txid = node.sendtoaddress(parent_address, 1.0)

        # Set the mocktime to the arrival time of the parent transaction.
        entry_time = node.getmempoolentry(parent_txid)['time']
        node.setmocktime(entry_time)

        # Create child transaction spending the parent transaction
        vout = find_vout_for_address(node, parent_txid, parent_address)
        inputs = [{'txid': parent_txid, 'vout': vout}]
        outputs = {node.getnewaddress(): 0.99}
        child_raw = node.createrawtransaction(inputs, outputs)
        child_signed = node.signrawtransactionwithwallet(child_raw)['hex']

        # Let half of the timeout elapse and broadcast the child transaction.
        half_expiry_time = entry_time + int(60 * 60 * timeout/2)
        node.setmocktime(half_expiry_time)
        child_txid = node.sendrawtransaction(child_signed)
        self.log.info('Broadcast child transaction after {} hours.'.format(
            timedelta(seconds=(half_expiry_time-entry_time))))

        # Let most of the timeout elapse and check that the parent tx is still
        # in the mempool.
        nearly_expiry_time = entry_time + 60 * 60 * timeout - 5
        node.setmocktime(nearly_expiry_time)
        # Expiry of mempool transactions is only checked when a new transaction
        # is added to the to the mempool.
        node.sendtoaddress(node.getnewaddress(), 1.0)
        self.log.info('Test parent tx not expired after {} hours.'.format(
            timedelta(seconds=(nearly_expiry_time-entry_time))))
        assert_equal(entry_time, node.getmempoolentry(parent_txid)['time'])

        # Transaction should be evicted from the mempool after the expiry time
        # has passed.
        expiry_time = entry_time + 60 * 60 * timeout + 5
        node.setmocktime(expiry_time)
        # Expiry of mempool transactions is only checked when a new transaction
        # is added to the to the mempool.
        node.sendtoaddress(node.getnewaddress(), 1.0)
        self.log.info('Test parent tx expiry after {} hours.'.format(
            timedelta(seconds=(expiry_time-entry_time))))
        assert_raises_rpc_error(-5, 'Transaction not in mempool',
                                node.getmempoolentry, parent_txid)

        # The child transaction should be removed from the mempool as well.
        self.log.info('Test child tx is evicted as well.')
        assert_raises_rpc_error(-5, 'Transaction not in mempool',
                                node.getmempoolentry, child_txid)

    def run_test(self):
        self.log.info('Test default mempool expiry timeout of %d hours.' %
                      DEFAULT_MEMPOOL_EXPIRY)
        self.test_transaction_expiry(DEFAULT_MEMPOOL_EXPIRY)

        self.log.info('Test custom mempool expiry timeout of %d hours.' %
                      CUSTOM_MEMPOOL_EXPIRY)
        self.restart_node(0, ['-mempoolexpiry=%d' % CUSTOM_MEMPOOL_EXPIRY])
        self.test_transaction_expiry(CUSTOM_MEMPOOL_EXPIRY)


if __name__ == '__main__':
    MempoolExpiryTest().main()
