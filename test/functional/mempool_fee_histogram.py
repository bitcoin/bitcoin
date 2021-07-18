#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool fee histogram."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_no_key,
)

class MempoolFeeHistogramTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        node.generate(102)

        # We have two utxos and we do this:
        #
        # coinbase-tx-101 <- tx1 (5 sat/vB) <- tx2 (14 sat/vB) <----\
        # coinbase-tx-102 <--------------------------------------- tx3 (6 sat/vB)

        self.log.info("Test getmempoolinfo does not return fee histogram by default")
        assert_no_key('fee_histogram', node.getmempoolinfo())

        self.log.info("Test getmempoolinfo returns empty fee histogram when mempool is empty")
        info = node.getmempoolinfo([1, 2, 3])
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(0, non_empty_groups)
        assert_equal(3, empty_groups)
        assert_equal(0, total_fees)

        self.log.info("Test that we have two spendable utxos and lock the second one")
        utxos = node.listunspent()
        assert_equal(2, len(utxos))
        node.lockunspent(False, [{"txid": utxos[1]["txid"], "vout": utxos[1]["vout"]}])

        self.log.info("Send tx1 transaction with 5 sat/vB fee rate")
        node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("50.0"), fee_rate=5, subtractfeefromamount=True)

        self.log.info("Test fee rate histogram when mempool contains 1 transaction (tx1: 5 sat/vB)")
        info = node.getmempoolinfo([1, 3, 5, 10])
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(1, non_empty_groups)
        assert_equal(3, empty_groups)
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['5']['count'])
        assert_equal(total_fees, info['fee_histogram']['total_fees'])

        self.log.info("Send tx2 transaction with 14 sat/vB fee rate (spends tx1 utxo)")
        node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("25.0"), fee_rate=14)

        self.log.info("Test fee rate histogram when mempool contains 2 transactions (tx1: 5 sat/vB, tx2: 14 sat/vB)")
        info = node.getmempoolinfo([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        # Both tx1 and tx2 are supposed to be reported in 8 sat/vB fee rate group
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(1, non_empty_groups)
        assert_equal(14, empty_groups)
        assert_equal(2, info['fee_histogram']['fee_rate_groups']['8']['count'])
        assert_equal(total_fees, info['fee_histogram']['total_fees'])

        # Unlock the second UTXO which we locked
        node.lockunspent(True, [{"txid": utxos[1]["txid"], "vout": utxos[1]["vout"]}])

        self.log.info("Send tx3 transaction with 6 sat/vB fee rate (spends all available utxos)")
        node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("99.9"), fee_rate=6)

        self.log.info("Test fee rate histogram when mempool contains 3 transactions (tx1: 5 sat/vB, tx2: 14 sat/vB, tx3: 6 sat/vB)")
        info = node.getmempoolinfo([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        # Verify that each of 6, 7 and 8 sat/vB fee rate groups contain one transaction
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(3, non_empty_groups)
        assert_equal(12, empty_groups)
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['6']['count'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['7']['count'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['8']['count'])
        assert_equal(total_fees, info['fee_histogram']['total_fees'])


    def histogram_stats(self, histogram):
        total_fees = 0
        empty_count = 0
        non_empty_count = 0

        for key, bin in histogram['fee_rate_groups'].items():
            assert_equal(int(key), bin['from'])
            if bin['fees'] > 0:
                assert_greater_than(bin['count'], 0)
            else:
                assert_equal(bin['count'], 0)
            assert_greater_than_or_equal(bin['fees'], 0)
            assert_greater_than_or_equal(bin['size'], 0)
            if bin['to'] is not None:
                assert_greater_than_or_equal(bin['to'], bin['from'])
            total_fees += bin['fees']

            if bin['count'] == 0:
                empty_count += 1
            else:
                non_empty_count += 1

        return (non_empty_count, empty_count, total_fees)

if __name__ == '__main__':
    MempoolFeeHistogramTest().main()
