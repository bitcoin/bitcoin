# !/usr/bin/env python3
# Copyright (c) 2014-2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool minimum fee persistence.

bitcoind will dump mempool minimum fee periodically and on shutdown.
On start, it will reload the maximum of persisted mempool minimum fee and
default minimum fee as the mempool minimum fee in the absence of
user-configured minrelaytxfee.
"""

from decimal import Decimal
import os
import time

from feature_fee_estimation import SECONDS_PER_HOUR
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

DEFAULT_MEMPOOL_MINIMUM_FEE = 0.00001000
DUMP_VERSION = 1
LOW_MEMPOOL_MINIMUM_FEE = 0.00000100
HIGH_MEMPOOL_MINIMUM_FEE = 0.00025000
MAX_MEMPOOL_MIN_FEE_AGE = 1

class MempoolMinFeePersistTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False, descriptors=True)

    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[f"-minrelaytxfee={HIGH_MEMPOOL_MINIMUM_FEE}"], []]

    def setup_network(self):
        self.add_nodes(2, extra_args=self.extra_args)
        self.start_nodes(extra_args=self.extra_args)
        self.node0 = self.nodes[0]
        self.wallet0 = MiniWallet(self.node0)
        self.node1 = self.nodes[1]

    def test_persistence_of_mempool_min_fee(self):
        # Test that the mempool_min fee is set initially
        assert_equal(float(self.node0.getmempoolinfo()["mempoolminfee"]), HIGH_MEMPOOL_MINIMUM_FEE)
        assert_equal(float(self.node1.getmempoolinfo()["mempoolminfee"]), DEFAULT_MEMPOOL_MINIMUM_FEE)

        # Test transactions with fee rate >= mempool minimum fee will be accepted into the Mempool
        high_fee_tx_hex = self.wallet0.create_self_transfer(fee_rate=Decimal(HIGH_MEMPOOL_MINIMUM_FEE * 2))["tx"].serialize().hex()
        res = self.node0.testmempoolaccept([high_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)
        res = self.node1.testmempoolaccept([high_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)

        # Test transactions with a fee rate < memPool minimum fee will not be accepted into node0 the Mempool
        default_fee_tx_hex = self.wallet0.create_self_transfer(fee_rate=Decimal(DEFAULT_MEMPOOL_MINIMUM_FEE))["tx"].serialize().hex()
        res = self.node0.testmempoolaccept([default_fee_tx_hex])[0]
        assert_equal(res['allowed'], False)

        # Since node1 has a low mempool minimum fee it will accept the low fee transaction into it's Mempool
        res = self.node1.testmempoolaccept([default_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)

        # Stop the nodes and test that the Mempool minimum fee will be flushed
        self.stop_nodes()

        # Verify that the string "Mempool minimum fee flushed to fee_estimates.dat." is present in both
        # nodes debug.log. If present, then the Mempool minimum fee was flushed.
        self.node0.assert_debug_log(expected_msgs=["Mempool minimum fee flushed to fee_estimates.dat."])
        self.node1.assert_debug_log(expected_msgs=["Mempool minimum fee flushed to fee_estimates.dat."])

        # Start the nodes without the minrelaytxfee option on node0 and verify that the persisted will be read
        self.start_nodes(extra_args=[[], []])
        self.node0.assert_debug_log(expected_msgs=["Mempool minimum fee read from fee_estimates.dat."])
        self.node1.assert_debug_log(expected_msgs=["Mempool minimum fee read from fee_estimates.dat."])

        assert_equal(float(self.node0.getmempoolinfo()["mempoolminfee"]), HIGH_MEMPOOL_MINIMUM_FEE)
        assert_equal(float(self.node1.getmempoolinfo()["mempoolminfee"]), DEFAULT_MEMPOOL_MINIMUM_FEE)

        self.log.info("Test after restart, transactions with fee rate lower than Mempool min fee will not be accepted into the Mempool")
        # Test transactions with enough fee rate
        res = self.node0.testmempoolaccept([high_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)
        res = self.node1.testmempoolaccept([high_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)

        # Test with a low fee rate lower than node0's mempool minimum fee
        res = self.node0.testmempoolaccept([default_fee_tx_hex])[0]
        assert_equal(res['allowed'], False)

        # Test with a low fee rate higher than node1's mempool minimum fee
        res = self.node1.testmempoolaccept([default_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)

    def test_not_reading_lower_mempool_min_fee(self):
        # Restart node0 with a low mempool minimum fee
        self.restart_node(0, extra_args=[f"-minrelaytxfee={LOW_MEMPOOL_MINIMUM_FEE:.8f}"])
        # Restart node1 with the persisted Mempool minimum fee
        self.restart_node(1)
        assert_equal(float(self.node0.getmempoolinfo()["mempoolminfee"]), LOW_MEMPOOL_MINIMUM_FEE)
        assert_equal(float(self.node1.getmempoolinfo()["mempoolminfee"]), DEFAULT_MEMPOOL_MINIMUM_FEE)

        # Create a low fee transaction and test that it will be accepted into node0's Mempool
        low_fee_tx_hex = self.wallet0.create_self_transfer(fee_rate=Decimal(LOW_MEMPOOL_MINIMUM_FEE))["tx"].serialize().hex()
        res = self.node0.testmempoolaccept([low_fee_tx_hex])[0]
        assert_equal(res['allowed'], True)

        # Test that the low fee transaction will not be accepted into node1
        res = self.node1.testmempoolaccept([low_fee_tx_hex])[0]
        assert_equal(res['allowed'], False)

        # Restart the nodes with the persisted mempool minimum fee
        self.restart_node(0, extra_args=[])
        self.restart_node(1, extra_args=[])

        # Test that the fee rate for node0 is back to default
        assert_equal(float(self.node0.getmempoolinfo()["mempoolminfee"]), DEFAULT_MEMPOOL_MINIMUM_FEE)
        assert_equal(float(self.node1.getmempoolinfo()["mempoolminfee"]), DEFAULT_MEMPOOL_MINIMUM_FEE)

    def test_mempool_min_fee_is_flushed_periodically(self):
        with self.nodes[0].assert_debug_log(expected_msgs=["Mempool minimum fee flushed to fee_estimates.dat."]):
            # Mock the scheduler for an hour, the mempool minimum fee will be flushed to fee_estimates.dat.
            self.nodes[0].mockscheduler(SECONDS_PER_HOUR)

    def test_reading_stale_mempool_min_fee(self):
        # Get the mempool minimum fee rate while node is running
        mempoolminfee = self.nodes[0].getmempoolinfo()["mempoolminfee"]

        # Restart node to ensure mempool minimum fee is read
        self.restart_node(0)
        assert_equal(self.nodes[0].getmempoolinfo()["mempoolminfee"], mempoolminfee)

        fee_dat = self.nodes[0].chain_path / "fee_estimates.dat"

        # Stop the node and backdate the fee_estimates.dat file more than MAX_MEMPOOL_MIN_FEE_AGE with a second
        self.stop_node(0)
        last_modified_time = time.time() - (((MAX_MEMPOOL_MIN_FEE_AGE) * SECONDS_PER_HOUR)  + 1 )
        os.utime(fee_dat, (last_modified_time, last_modified_time))

        # Start node and ensure the mempool minimum fee file was not read and is back to default
        self.start_node(0)
        assert_equal(float(self.node1.getmempoolinfo()["mempoolminfee"]), DEFAULT_MEMPOOL_MINIMUM_FEE)

    def run_test(self):
        self.log.info("Test that the mempool minimum fee will be persisted to fee_estimates.dat.")
        self.test_persistence_of_mempool_min_fee()

        self.log.info("Test that the persisted mempool minimum fee lower than default mempool minimum fee will not be read.")
        self.test_not_reading_lower_mempool_min_fee()

        self.log.info("Test that the mempool minimum fee will be flushed to disk periodically.")
        self.test_mempool_min_fee_is_flushed_periodically()

        self.log.info("Test stale mempool minimum fee will not be read")
        self.restart_node(0, extra_args=[f"-minrelaytxfee={HIGH_MEMPOOL_MINIMUM_FEE:.8f}"])
        self.test_reading_stale_mempool_min_fee()



if __name__ == "__main__":
    MempoolMinFeePersistTest().main()
