#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test covering the Year 2038 problem.

https://en.wikipedia.org/wiki/Year_2038_problem
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

EPOCH_2038_BEFORE = 2147440306 # (GMT): Monday, January 18, 2038 3:11:46 PM
EPOCH_2038_AFTER = 2147613106 # (GMT): Wednesday, January 20, 2038 3:11:46 PM
EPOCH_2106 = 4293404400000


class Y2038EpochTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 2

    def test_y2038(self):

        for epoch in [EPOCH_2038_BEFORE, EPOCH_2038_AFTER, EPOCH_2106]:
            # mocks the time to the epoch
            self.nodes[0].setmocktime(epoch)
            self.nodes[1].setmocktime(epoch)

            # Generates a blocks
            self.nodes[0].generate(nblocks=1)

            self.log.info(epoch)
            self.log.info(time.time())

            self.sync_all()

            assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())

    def run_test(self):
        self.log.info('Test about Y2038')
        self.test_y2038()

if __name__ == '__main__':
    Y2038EpochTest().main()
