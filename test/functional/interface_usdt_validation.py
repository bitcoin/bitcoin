#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Tests the validation:* tracepoint API interface.
    See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-validation
"""

import time

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.usdt import USDT
from test_framework.util import (
    assert_equal,
)


class ValidationTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_usdt_tools()
        self.skip_if_running_under_valgrind()

    def start_nodes(self):
        self.trace = USDT(self.nodes[0], {
            "validation:block_connected": [
                ("hash", "h"), ("height", "i"), ("transactions", "i"),
                ("inputs", "i"), ("sigops", "i"), ("duration", "i"),
            ],
        })
        super().start_nodes()

    def run_test(self):
        # Tests the validation:block_connected tracepoint by generating blocks
        # and comparing the values passed in the tracepoint arguments with the
        # blocks.
        # See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#tracepoint-validationblock_connected

        BLOCKS_EXPECTED = 2
        expected_blocks = dict()
        self.trace.clear()

        self.log.info(f"mine {BLOCKS_EXPECTED} blocks")
        generatetoaddress_duration = dict()
        for _ in range(BLOCKS_EXPECTED):
            start = time.time()
            hash = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)[0]
            generatetoaddress_duration[hash] = (time.time() - start) * 1e9  # in nanoseconds
            expected_blocks[hash] = self.nodes[0].getblock(hash, 2)

        events = self.trace.read()

        self.log.info(f"check that we correctly traced {BLOCKS_EXPECTED} blocks")
        for event in events:
            block_hash = event.hash
            block = expected_blocks[block_hash]
            assert_equal(block["hash"], block_hash)
            assert_equal(block["height"], event.height)
            assert_equal(len(block["tx"]), event.transactions)
            assert_equal(len([tx["vin"] for tx in block["tx"]]), event.inputs)
            assert_equal(0, event.sigops)  # no sigops in coinbase tx
            # only plausibility checks
            assert event.duration > 0
            # generatetoaddress (mining and connecting) takes longer than
            # connecting the block. In case the duration unit is off, we'll
            # detect it with this assert.
            assert event.duration < generatetoaddress_duration[block_hash]
            del expected_blocks[block_hash]
        assert_equal(BLOCKS_EXPECTED, len(events))
        assert_equal(0, len(expected_blocks))


if __name__ == '__main__':
    ValidationTracepointTest(__file__).main()
