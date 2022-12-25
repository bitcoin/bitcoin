#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RPC call related to the uptime command.

Test corresponds to code in rpc/server.cpp.
"""

import time

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_raises_rpc_error


class UptimeTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self._test_negative_time()
        self._test_uptime()

    def _test_negative_time(self):
        assert_raises_rpc_error(-8, "Mocktime cannot be negative: -1.", self.nodes[0].setmocktime, -1)

    def _test_uptime(self):
        wait_time = 10
        self.nodes[0].setmocktime(int(time.time() + wait_time))
        assert self.nodes[0].uptime() >= wait_time


if __name__ == '__main__':
    UptimeTest().main()
