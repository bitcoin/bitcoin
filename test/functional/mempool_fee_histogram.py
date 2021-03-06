#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool fee histogram."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)

class MempoolFeeHistogramTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        self.generate(self.nodes[0], COINBASE_MATURITY + 2, sync_fun=self.no_op)

        # We have two UTXOs (utxo_1 and utxo_2) and we create three changeless transactions:
        # - tx1 (5 sat/vB): spending utxo_1
        # - tx2 (14 sat/vB): spending output from tx1
        # - tx3 (6 sat/vB): spending utxo_2 and the output from tx2

        self.log.info("Test getmempoolinfo does not return fee histogram by default")
        assert ("fee_histogram" not in node.getmempoolinfo())

        self.log.info("Test getmempoolinfo returns empty fee histogram when mempool is empty")
        info = node.getmempoolinfo([1, 2, 3])

        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(0, non_empty_groups)
        assert_equal(3, empty_groups)
        assert_equal(0, total_fees)

        for i in ['1', '2', '3']:
            assert_equal(0, info['fee_histogram']['fee_rate_groups'][i]['size'])
            assert_equal(0, info['fee_histogram']['fee_rate_groups'][i]['count'])
            assert_equal(0, info['fee_histogram']['fee_rate_groups'][i]['fees'])
            assert_equal(int(i), info['fee_histogram']['fee_rate_groups'][i]['from'])

        self.log.info("Test that we have two spendable UTXOs and lock the second one")
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

        assert_equal(0, info['fee_histogram']['fee_rate_groups']['1']['size'])
        assert_equal(0, info['fee_histogram']['fee_rate_groups']['1']['count'])
        assert_equal(0, info['fee_histogram']['fee_rate_groups']['1']['fees'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['1']['from'])

        assert_equal(0, info['fee_histogram']['fee_rate_groups']['3']['size'])
        assert_equal(0, info['fee_histogram']['fee_rate_groups']['3']['count'])
        assert_equal(0, info['fee_histogram']['fee_rate_groups']['3']['fees'])
        assert_equal(3, info['fee_histogram']['fee_rate_groups']['3']['from'])

        assert_equal(188, info['fee_histogram']['fee_rate_groups']['5']['size'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['5']['count'])
        assert_equal(940, info['fee_histogram']['fee_rate_groups']['5']['fees'])
        assert_equal(5, info['fee_histogram']['fee_rate_groups']['5']['from'])

        assert_equal(0, info['fee_histogram']['fee_rate_groups']['10']['size'])
        assert_equal(0, info['fee_histogram']['fee_rate_groups']['10']['count'])
        assert_equal(0, info['fee_histogram']['fee_rate_groups']['10']['fees'])
        assert_equal(10, info['fee_histogram']['fee_rate_groups']['10']['from'])

        self.log.info("Send tx2 transaction with 14 sat/vB fee rate (spends tx1 UTXO)")
        node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("25.0"), fee_rate=14, subtractfeefromamount=True)

        self.log.info("Test fee rate histogram when mempool contains 2 transactions (tx1: 5 sat/vB, tx2: 14 sat/vB)")
        info = node.getmempoolinfo([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        # Verify that tx1 and tx2 are reported in 5 sat/vB and 14 sat/vB in fee rate groups respectively
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(2, non_empty_groups)
        assert_equal(13, empty_groups)
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['5']['count'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['14']['count'])
        assert_equal(total_fees, info['fee_histogram']['total_fees'])

        # Unlock the second UTXO which we locked
        node.lockunspent(True, [{"txid": utxos[1]["txid"], "vout": utxos[1]["vout"]}])

        self.log.info("Send tx3 transaction with 6 sat/vB fee rate (spends all available UTXOs)")
        node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("99.9"), fee_rate=6, subtractfeefromamount=True)

        self.log.info("Test fee rate histogram when mempool contains 3 transactions (tx1: 5 sat/vB, tx2: 14 sat/vB, tx3: 6 sat/vB)")
        info = node.getmempoolinfo([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        # Verify that each of 5, 6 and 14 sat/vB fee rate groups contain one transaction
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(3, non_empty_groups)
        assert_equal(12, empty_groups)

        for i in ['1', '2', '3', '4', '7', '8', '9', '10', '11', '12', '13', '15']:
            assert_equal(0, info['fee_histogram']['fee_rate_groups'][i]['size'])
            assert_equal(0, info['fee_histogram']['fee_rate_groups'][i]['count'])
            assert_equal(0, info['fee_histogram']['fee_rate_groups'][i]['fees'])
            assert_equal(int(i), info['fee_histogram']['fee_rate_groups'][i]['from'])

        assert_equal(188, info['fee_histogram']['fee_rate_groups']['5']['size'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['5']['count'])
        assert_equal(940, info['fee_histogram']['fee_rate_groups']['5']['fees'])
        assert_equal(5, info['fee_histogram']['fee_rate_groups']['5']['from'])

        assert_equal(356, info['fee_histogram']['fee_rate_groups']['6']['size'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['6']['count'])
        assert_equal(2136, info['fee_histogram']['fee_rate_groups']['6']['fees'])
        assert_equal(6, info['fee_histogram']['fee_rate_groups']['6']['from'])

        assert_equal(141, info['fee_histogram']['fee_rate_groups']['14']['size'])
        assert_equal(1, info['fee_histogram']['fee_rate_groups']['14']['count'])
        assert_equal(1974, info['fee_histogram']['fee_rate_groups']['14']['fees'])
        assert_equal(14, info['fee_histogram']['fee_rate_groups']['14']['from'])

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
            total_fees += bin['fees']

            if bin['count'] == 0:
                empty_count += 1
            else:
                non_empty_count += 1

        return (non_empty_count, empty_count, total_fees)

if __name__ == '__main__':
    MempoolFeeHistogramTest().main()
