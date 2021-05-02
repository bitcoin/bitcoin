#!/usr/bin/env python3
# Copyright (c) 2016-2019 The XBit Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test using named arguments for RPCs."""

from test_framework.test_framework import XBitTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class NamedArgumentTest(XBitTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]
        h = node.help(command='getblockchaininfo')
        assert h.startswith('getblockchaininfo\n')

        assert_raises_rpc_error(-8, 'Unknown named parameter', node.help, random='getblockchaininfo')

        h = node.getblockhash(height=0)
        node.getblock(blockhash=h)

        assert_equal(node.echo(), [])
        assert_equal(node.echo(arg0=0,arg9=9), [0] + [None]*8 + [9])
        assert_equal(node.echo(arg1=1), [None, 1])
        assert_equal(node.echo(arg9=None), [None]*10)
        assert_equal(node.echo(arg0=0,arg3=3,arg9=9), [0] + [None]*2 + [3] + [None]*5 + [9])

if __name__ == '__main__':
    NamedArgumentTest().main()
