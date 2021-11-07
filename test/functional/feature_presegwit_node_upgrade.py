#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test a pre-segwit node upgrading to segwit consensus"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    softfork_active,
)

class SegwitUpgradeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-segwitheight=10"]]

    def run_test(self):
        """A pre-segwit node with insufficiently validated blocks needs to redownload blocks"""

        self.log.info("Testing upgrade behaviour for pre-segwit node to segwit rules")
        node = self.nodes[0]

        # Node hasn't been used or connected yet
        assert_equal(node.getblockcount(), 0)

        assert not softfork_active(node, "segwit")

        # Generate 8 blocks without witness data
        node.generate(8)
        assert_equal(node.getblockcount(), 8)

        self.stop_node(0)
        # Restarting the node (with segwit activation height set to 5) should result in a shutdown
        # because the blockchain consists of 3 insufficiently validated blocks per segwit consensus rules.
        node.assert_start_raises_init_error(
                extra_args=["-segwitheight=5"],
                expected_msg=": Witness data for blocks after height 5 requires validation. Please restart with -reindex..\nPlease restart with -reindex or -reindex-chainstate to recover.")

        # As directed, the user restarts the node with -reindex
        self.start_node(0, extra_args=["-reindex", "-segwitheight=5"])

        # With the segwit consensus rules, the node is able to validate only up to block 4
        assert_equal(node.getblockcount(), 4)

        # The upgraded node should now have segwit activated
        assert softfork_active(node, "segwit")


if __name__ == '__main__':
    SegwitUpgradeTest().main()
