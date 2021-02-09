#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test node rebroadcast logic.

We start by creating a set of transactions to set the rebroadcast minimum fee
cache to a medium fee rate value.

We then create three sets of transactions:
    1. aged, high fee rate
    2. aged, low fee rate
    3. recent, high fee rate

We add a new connection, trigger the rebroadcast functionality by mining an
empty block, check that the aged high fee rate transactions were succesfully
rebroadcast, and that the other two sets were not rebroadcast.
"""

import time
from decimal import Decimal

from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_greater_than,
    create_confirmed_utxos,
)

# Constant from consensus.h
MAX_BLOCK_WEIGHT = 4000000


class NodeRebroadcastTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[
            "-whitelist=noban@127.0.0.1",
            "-txindex",
            "-rebroadcast=1"
        ]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def make_txn_at_fee_rate(self, input_utxo, outputs, outputs_sum, desired_fee_rate, change_address):
        node = self.nodes[0]
        node1 = self.nodes[1]

        inputs = [{'txid': input_utxo['txid'], 'vout': input_utxo['vout']}]

        # Calculate how much input values add up to
        input_tx_hsh = input_utxo['txid']
        raw_tx = node.getrawtransaction(input_tx_hsh, True)
        inputs_list = raw_tx['vout']
        if 'coinbase' in raw_tx['vin'][0].keys():
            return
        index = raw_tx['vin'][0]['vout']
        inputs_sum = inputs_list[index]['value']

        # Divide by 1000 because vsize is in bytes & cache fee rate is BTC / kB
        tx_vsize_with_change = 1660
        desired_fee_btc = desired_fee_rate * tx_vsize_with_change / 1000
        current_fee_btc = inputs_sum - Decimal(str(outputs_sum))

        # Add another output with change
        outputs[change_address] = float(current_fee_btc - desired_fee_btc)
        outputs_sum += outputs[change_address]

        # Form transaction & submit to mempool of both nodes directly
        raw_tx_hex = node.createrawtransaction(inputs, outputs)
        signed_tx = node.signrawtransactionwithwallet(raw_tx_hex)
        tx_hsh = node.sendrawtransaction(hexstring=signed_tx['hex'], maxfeerate=0)
        node1.sendrawtransaction(hexstring=signed_tx['hex'], maxfeerate=0)

        # Retrieve mempool transaction to calculate fee rate
        mempool_entry = node.getmempoolentry(tx_hsh)

        # Check absolute fee matches up to expectations
        fee_calculated = inputs_sum - Decimal(str(outputs_sum))
        fee_got = mempool_entry['fee']
        assert_approx(float(fee_calculated), float(fee_got))

        # mempool_entry['fee'] is in BTC, fee rate should be BTC / kb
        fee_rate = mempool_entry['fee'] * 1000 / mempool_entry['vsize']
        assert_approx(float(fee_rate), float(desired_fee_rate))

        return tx_hsh

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Make transactions to set the cache fee rate")
        # Create UTXOs that we can spend
        min_relay_fee = node.getnetworkinfo()["relayfee"]
        utxos = create_confirmed_utxos(min_relay_fee, node, 2000)

        addresses = []
        for _ in range(50):
            addresses.append(node.getnewaddress())

        # Create large transactions by sending to all the addresses
        outputs = {addr: 0.0001 for addr in addresses}
        change_address = node.getnewaddress()
        outputs_sum = 0.0001 * 50

        self.sync_mempools()

        # Create lots of cache_fee_rate transactions with that large output
        cache_fee_rate = min_relay_fee * 3
        for _ in range(len(utxos) - 1000):
            self.make_txn_at_fee_rate(utxos.pop(), outputs, outputs_sum, cache_fee_rate, change_address)

        self.sync_mempools()

        # Confirm we've created enough transactions to fill a cache block,
        # ensuring the threshold fee rate will be cache_fee_rate
        # Divide by 4 to convert from weight to virtual bytes
        assert_greater_than(node.getmempoolinfo()['bytes'], MAX_BLOCK_WEIGHT / 4)

        # Make transactions to later mine into a block
        block_txns = []
        for _ in range(600):
            block_txns.append(self.make_txn_at_fee_rate(utxos.pop(), outputs, outputs_sum, cache_fee_rate, change_address))
        block_txns = list(filter(None, block_txns))

        # CacheMinRebroadcastFee is scheduled to run every minute
        # Bump the scheduled jobs by slightly over a minute to trigger it
        node.mockscheduler(62)

        self.log.info("Make high fee-rate transactions")
        high_fee_rate_tx_hshs = []
        high_fee_rate = min_relay_fee * 4

        for _ in range(10):
            tx_hsh = self.make_txn_at_fee_rate(utxos.pop(), outputs, outputs_sum, high_fee_rate, change_address)
            high_fee_rate_tx_hshs.append(tx_hsh)

        self.log.info("Make low fee-rate transactions")
        low_fee_rate_tx_hshs = []
        low_fee_rate = min_relay_fee * 2

        for _ in range(10):
            tx_hsh = self.make_txn_at_fee_rate(utxos.pop(), outputs, outputs_sum, low_fee_rate, change_address)
            low_fee_rate_tx_hshs.append(tx_hsh)

        # Ensure these transactions are removed from the unbroadcast set. Or in
        # other words, that all GETDATAs have been received before its time to
        # rebroadcast
        self.sync_mempools()

        # Confirm that the remaining bytes in the mempool are less than what
        # fits in the rebroadcast block. Otherwise, we could get a false
        # positive where the low_fee_rate transactions are not rebroadcast
        # simply because they do not fit, not because they were filtered out.
        assert_greater_than(3 * MAX_BLOCK_WEIGHT / 4, node.getmempoolinfo()['bytes'])

        # Bump time forward to ensure existing transactions meet
        # REBROADCAST_MIN_TX_AGE.
        node.setmocktime(int(time.time()) + 35 * 60)

        self.log.info("Make recent transactions")
        recent_tx_hshs = []

        for _ in range(10):
            tx_hsh = self.make_txn_at_fee_rate(utxos.pop(), outputs, outputs_sum, high_fee_rate, change_address)
            recent_tx_hshs.append(tx_hsh)

        self.log.info("Trigger rebroadcast by mining a block")
        conn = node.add_p2p_connection(P2PTxInvStore())

        block_hash = self.nodes[0].generateblock(output=node.getnewaddress(), transactions=block_txns)

        # We identify rebroadcast transaction canadidates by mining a block
        # with 3/4 the weight of the block received from the network. Ensure
        # that this test setup triggers rebroadcast functionality with a
        # sufficiently large block.
        assert(self.nodes[0].getblock(block_hash['hash'])['weight'] > 3000000)

        self.wait_until(lambda: conn.get_invs(), timeout=30)
        rebroadcasted_invs = conn.get_invs()

        self.log.info("Check that high fee rate transactions are rebroadcast")
        for txhsh in high_fee_rate_tx_hshs:
            wtxhsh = node.getmempoolentry(txhsh)['wtxid']
            wtxid = int(wtxhsh, 16)
            assert(wtxid in rebroadcasted_invs)

        self.log.info("Check that low fee rate transactions are NOT rebroadcast")
        for txhsh in low_fee_rate_tx_hshs:
            wtxhsh = node.getmempoolentry(txhsh)['wtxid']
            wtxid = int(wtxhsh, 16)
            assert(wtxid not in rebroadcasted_invs)

        self.log.info("Check that recent transactions are NOT rebroadcast")
        for txhsh in recent_tx_hshs:
            wtxhsh = node.getmempoolentry(txhsh)['wtxid']
            wtxid = int(wtxhsh, 16)
            assert(wtxid not in rebroadcasted_invs)


if __name__ == '__main__':
    NodeRebroadcastTest().main()
