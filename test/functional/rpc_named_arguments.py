#!/usr/bin/env python3
# Copyright (c) 2016-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test using named arguments for RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class NamedArgumentTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        h = node.help(command='getblockchaininfo')
        assert h.startswith('getblockchaininfo\n')

        assert_raises_rpc_error(-8, 'Unknown named parameter', node.help, random='getblockchaininfo')

        h = node.getblockhash(height=0)
        node.getblock(blockhash=h)

        assert_equal(node.echojson(), [])
        assert_equal(node.echojson(arg0=0, arg9=9), [0] + [None] * 8 + [9])
        assert_equal(node.echojson(arg1=1), [None, 1])
        assert_equal(node.echojson(arg9=None), [] if self.options.usecli else [None] * 10)
        assert_equal(node.echojson(arg0=0, arg3=3, arg9=9), [0] + [None] * 2 + [3] + [None] * 5 + [9])
        assert_equal(node.echojson(0, 1, arg3=3, arg5=5), [0, 1, None, 3, None, 5])
        assert_raises_rpc_error(-8, "Parameter arg1 specified twice both as positional and named argument", node.echojson, 0, 1, arg1=1)
        assert_raises_rpc_error(-8, "Parameter arg1 specified twice both as positional and named argument", node.echojson, 0, None, 2, arg1=1)

        self.log.info("Test getmempoolinfo options")
        default_info = node.getmempoolinfo()
        for options in [None, {}, {"sat_vb": None}, {"sat_vb": False}]:
            assert_equal(node.getmempoolinfo(options), default_info)
        for sat_vb in [None, False, True]:
            assert_equal(
                node.getmempoolinfo({"sat_vb": sat_vb}),
                node.getmempoolinfo(options={"sat_vb": sat_vb}),
                node.getmempoolinfo(sat_vb=sat_vb),
            )
        assert_raises_rpc_error(-3, "Wrong type passed", node.getmempoolinfo, True)
        assert_raises_rpc_error(-3, "JSON value of type number for field sat_vb is not of expected type bool", node.getmempoolinfo, {"sat_vb": 1})
        assert_raises_rpc_error(-3, "JSON value of type number for field sat_vb is not of expected type bool", node.getmempoolinfo, sat_vb=1)
        assert_raises_rpc_error(-3, "Unexpected key unknown", node.getmempoolinfo, {"unknown": True})
        assert_raises_rpc_error(-8, "Parameter options conflicts with parameter sat_vb", node.getmempoolinfo, options={"sat_vb": False}, sat_vb=True)

if __name__ == '__main__':
    NamedArgumentTest(__file__).main()
