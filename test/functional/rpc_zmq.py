#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test for the ZMQ RPC methods."""

from test_framework.test_framework import (
    BitcoinTestFramework, skip_if_no_py3_zmq, skip_if_no_bitcoind_zmq)
from test_framework.util import assert_equal


class RPCZMQTest(BitcoinTestFramework):

    address = "tcp://127.0.0.1:28332"

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        skip_if_no_py3_zmq()
        skip_if_no_bitcoind_zmq(self)
        self._test_getzmqnotifications()

    def _test_getzmqnotifications(self):
        self.restart_node(0, extra_args=[])
        assert_equal(self.nodes[0].getzmqnotifications(), [])

        self.restart_node(0, extra_args=["-zmqpubhashtx=%s" % self.address])
        assert_equal(self.nodes[0].getzmqnotifications(), [
            {"type": "pubhashtx", "address": self.address},
        ])


if __name__ == '__main__':
    RPCZMQTest().main()
