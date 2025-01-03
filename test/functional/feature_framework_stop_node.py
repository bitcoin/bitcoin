#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify that when a node process fails due to a startup error, we can still call
TestNode.stop_node() without triggering knock-on errors.
"""

from test_framework.test_node import (
    FailedToStartError,
    TestNode,
)
from test_framework.util import (
    assert_raises_message,
)
from test_framework.test_framework import BitcoinTestFramework

class FeatureFrameworkStopNode(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    # Overridden to avoid syncing non-started nodes.
    def setup_network(self):
        self.setup_nodes()

    # Overridden to avoid starting nodes before run_test.
    def setup_nodes(self):
        self.add_nodes(self.num_nodes, self.extra_args)

    def run_test(self):
        # We don't call assert_start_raises_init_error here, as we want
        # to exercise the more common wait_for_rpc_connection flow.
        self.nodes[0].start(extra_args=['-nonexistentarg'])
        invalid_arg_error = "Error: Error parsing command line arguments: Invalid parameter -nonexistentarg"
        assert_raises_message(
            exc=FailedToStartError,
            message="[node 0] bitcoind exited with status 1 during initialization. " + invalid_arg_error,
            fun=TestNode.wait_for_rpc_connection,
            self=self.nodes[0]
        )
        assert self.nodes[0].running, \
            "The process should still be flagged as running before calling stop_node()"
        self.log.debug("Explicitly stopping the node to verify it completes cleanly during the test")
        self.nodes[0].stop_node(expected_stderr=invalid_arg_error)

if __name__ == '__main__':
    FeatureFrameworkStopNode(__file__).main()
