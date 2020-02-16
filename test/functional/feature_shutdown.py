#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoind shutdown."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy, wait_until
from threading import Thread

def test_long_call(node):
    block = node.waitfornewblock()
    assert_equal(block['height'], 0)

class ShutdownTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.supports_cli = False

    def run_test(self):
        node = get_rpc_proxy(self.nodes[0].url, 1, timeout=600, coveragedir=self.nodes[0].coverage_dir)
        # Force connection establishment by executing a dummy command.
        node.getblockcount()
        Thread(target=test_long_call, args=(node,)).start()
        # Wait until the server is executing the above `waitfornewblock`.
        wait_until(lambda: len(self.nodes[0].getrpcinfo()['active_commands']) == 2)
        # Wait 1 second after requesting shutdown but not before the `stop` call
        # finishes. This is to ensure event loop waits for current connections
        # to close.
        self.stop_node(0, wait=1000)

if __name__ == '__main__':
    ShutdownTest().main()
