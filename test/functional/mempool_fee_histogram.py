#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool fee histogram."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
    COIN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)

def get_actual_fee_rate(fee_in_satoshis, vsize):
    # NOTE: Must round down, unlike ceildiv/get_fee
    fee_rate = fee_in_satoshis // vsize
    return str(fee_rate)

def get_tx_details(node, txid):
    info = node.gettransaction(txid=txid)
    info.update(node.getrawtransaction(txid=txid, verbose=True))
    info['fee'] = int(-info['fee'] * COIN)  # convert to satoshis
    info['feerate'] = get_actual_fee_rate(info['fee'], info['vsize'])
    return info

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
            assert_equal(0, info['fee_histogram'][i]['sizes'])
            assert_equal(0, info['fee_histogram'][i]['count'])
            assert_equal(0, info['fee_histogram'][i]['fees'])
            assert_equal(int(i), info['fee_histogram'][i]['from_feerate'])

        self.log.info("Test that we have two spendable UTXOs and lock the second one")
        utxos = node.listunspent()
        assert_equal(2, len(utxos))
        node.lockunspent(False, [{"txid": utxos[1]["txid"], "vout": utxos[1]["vout"]}])

        self.log.info("Send tx1 transaction with 5 sat/vB fee rate")
        tx1_txid = node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("50.0"), fee_rate=5, subtractfeefromamount=True)
        tx1_info = get_tx_details(node, tx1_txid)

        self.log.info(f"Test fee rate histogram when mempool contains 1 transaction (tx1: {tx1_info['feerate']} sat/vB)")
        info = node.getmempoolinfo([1, 3, 5, 10])
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        assert_equal(1, non_empty_groups)
        assert_equal(3, empty_groups)
        assert_equal(1, info['fee_histogram'][tx1_info['feerate']]['count'])
        assert_equal(total_fees, info['fee_histogram']['total_fees'])

        assert_equal(0, info['fee_histogram']['1']['sizes'])
        assert_equal(0, info['fee_histogram']['1']['count'])
        assert_equal(0, info['fee_histogram']['1']['fees'])
        assert_equal(1, info['fee_histogram']['1']['from_feerate'])

        assert_equal(0, info['fee_histogram']['3']['sizes'])
        assert_equal(0, info['fee_histogram']['3']['count'])
        assert_equal(0, info['fee_histogram']['3']['fees'])
        assert_equal(3, info['fee_histogram']['3']['from_feerate'])

        assert_equal(tx1_info['vsize'], info['fee_histogram']['5']['sizes'])
        assert_equal(1, info['fee_histogram']['5']['count'])
        assert_equal(tx1_info['fee'], info['fee_histogram']['5']['fees'])
        assert_equal(5, info['fee_histogram']['5']['from_feerate'])

        assert_equal(0, info['fee_histogram']['10']['sizes'])
        assert_equal(0, info['fee_histogram']['10']['count'])
        assert_equal(0, info['fee_histogram']['10']['fees'])
        assert_equal(10, info['fee_histogram']['10']['from_feerate'])

        self.log.info("Send tx2 transaction with 14 sat/vB fee rate (spends tx1 UTXO)")
        tx2_txid = node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("25.0"), fee_rate=14, subtractfeefromamount=True)
        tx2_info = get_tx_details(node, tx2_txid)

        self.log.info(f"Test fee rate histogram when mempool contains 2 transactions (tx1: {tx1_info['feerate']} sat/vB, tx2: {tx2_info['feerate']} sat/vB)")
        info = node.getmempoolinfo([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        # Verify that both tx1 and tx2 are reported in 8 sat/vB fee rate group
        (non_empty_groups, empty_groups, total_fees) = self.histogram_stats(info['fee_histogram'])
        tx1p2_feerate = get_actual_fee_rate(tx1_info['fee'] + tx2_info['fee'], tx1_info['vsize'] + tx2_info['vsize'])
        assert_equal(1, non_empty_groups)
        assert_equal(14, empty_groups)
        assert_equal(2, info['fee_histogram'][tx1p2_feerate]['count'])
        assert_equal(total_fees, info['fee_histogram']['total_fees'])

        # Unlock the second UTXO which we locked
        node.lockunspent(True, [{"txid": utxos[1]["txid"], "vout": utxos[1]["vout"]}])

        self.log.info("Send tx3 transaction with 6 sat/vB fee rate (spends all available UTXOs)")
        tx3_txid = node.sendtoaddress(address=node.getnewaddress(), amount=Decimal("99.9"), fee_rate=6, subtractfeefromamount=True)
        tx3_info = get_tx_details(node, tx3_txid)

        self.log.info(f"Test fee rate histogram when mempool contains 3 transactions (tx1: {tx1_info['feerate']} sat/vB, tx2: {tx2_info['feerate']} sat/vB, tx3: {tx3_info['feerate']} sat/vB)")
        info = node.getmempoolinfo([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        # Verify that each of 6, 8 and 9 sat/vB fee rate groups contain one transaction
        # tx1 should be grouped with tx2 + tx3 (descendants)
        # tx2 should be grouped with tx1 (ancestors only)
        # tx3 should be alone
        expected_histogram = dict(
            tuple(
                (str(n), {
                    'from_feerate': n,
                    'to_feerate': n + 1,
                    'count': 0,
                    'fees': 0,
                    'sizes': 0,
                }) for n in range(1, 16)
            ) + (
                ('total_fees', tx1_info['fee'] + tx2_info['fee'] + tx3_info['fee']),
            )
        )
        expected_frg = expected_histogram
        expected_frg['15']['to_feerate'] = 9223372036854775807
        tx1p2p3_feerate = get_actual_fee_rate(expected_histogram['total_fees'], tx1_info['vsize'] + tx2_info['vsize'] + tx3_info['vsize'])
        def inc_expected(feerate, txinfo):
            this_frg = expected_frg[feerate]
            this_frg['count'] += 1
            this_frg['fees'] += txinfo['fee']
            this_frg['sizes'] += txinfo['vsize']
        inc_expected(tx1p2p3_feerate, tx1_info)
        inc_expected(tx1p2_feerate, tx2_info)
        inc_expected(tx3_info['feerate'], tx3_info)

        assert_equal(expected_histogram, info['fee_histogram'])

        self.log.info("Test fee rate histogram with default groups")
        info = node.getmempoolinfo(with_fee_histogram=True)

        # Verify that the 6 sat/vB fee rate group has one transaction, and the 8-9 sat/vB fee rate group has two
        for collapse_n in (9, 11, 13, 15):
            for field in ('count', 'sizes', 'fees'):
                expected_frg[str(collapse_n - 1)][field] += expected_frg[str(collapse_n)][field]
            expected_frg[str(collapse_n - 1)]['to_feerate'] += 1
            del expected_frg[str(collapse_n)]
        expected_frg['14']['to_feerate'] += 1  # 16 is also skipped

        for new_n in (17, 20, 25) + tuple(range(30, 90, 10)) + (100, 120, 140, 170, 200, 250) + tuple(range(300, 900, 100)) + (1000, 1200, 1400, 1700, 2000, 2500) + tuple(range(3000, 9000, 1000)) + (10000,):
            frinfo = info['fee_histogram'][str(new_n)]
            assert frinfo['to_feerate'] > frinfo['from_feerate']
            del frinfo['to_feerate']
            assert_equal(frinfo, {
                'from_feerate': new_n,
                'count': 0,
                'fees': 0,
                'sizes': 0,
            })
            del info['fee_histogram'][str(new_n)]
        assert_equal(expected_histogram, info['fee_histogram'])

        self.log.info("Test getmempoolinfo(with_fee_histogram=False) does not return fee histogram")
        assert('fee_histogram' not in node.getmempoolinfo(with_fee_histogram=False))

    def histogram_stats(self, histogram):
        total_fees = 0
        empty_count = 0
        non_empty_count = 0

        for key, bin in histogram.items():
            if key == 'total_fees':
                continue
            assert_equal(int(key), bin['from_feerate'])
            if bin['fees'] > 0:
                assert_greater_than(bin['count'], 0)
            else:
                assert_equal(bin['count'], 0)
            assert_greater_than_or_equal(bin['fees'], 0)
            assert_greater_than_or_equal(bin['sizes'], 0)
            if bin['to_feerate'] is not None:
                assert_greater_than_or_equal(bin['to_feerate'], bin['from_feerate'])
                for next_key in sorted((*(int(a) for a in histogram.keys() if a != 'total_fees'), 0x7fffffffffffffff)):
                    if int(next_key) <= int(key):
                        continue
                    assert_equal(bin['to_feerate'], int(next_key))
                    break
            total_fees += bin['fees']

            if bin['count'] == 0:
                empty_count += 1
            else:
                non_empty_count += 1

        return (non_empty_count, empty_count, total_fees)

if __name__ == '__main__':
    MempoolFeeHistogramTest(__file__).main()
