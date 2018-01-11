#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Downgrade functional test

Test various downgrade scenarios. Compile the previous node binaries:

contrib/devtools/previous_release.sh -f v0.15.1 v0.14.2

Due to RPC changes introduced in v0.14 the below tests won't work for older
versions without some patches or workarounds.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    sync_blocks
)

class DowngradeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            [],
            ["-vbparams=segwit:0:0", "-vbparams=csv:0:0"], # v0.15.1
            ["-vbparams=segwit:0:0", "-vbparams=csv:0:0"] # v0.14.2
        ]

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, binary=[
            None,
            "build/releases/v0.15.1/src/bitcoind",
            "build/releases/v0.14.2/src/bitcoind"
        ])
        self.start_nodes()

    def run_test(self):
        self.nodes[0].generate(101)

        sync_blocks(self.nodes)

        # Santity check the test framework:
        res = self.nodes[self.num_nodes - 1].getblockchaininfo()
        assert_equal(res['blocks'], 101)
if __name__ == '__main__':
    DowngradeTest().main()
