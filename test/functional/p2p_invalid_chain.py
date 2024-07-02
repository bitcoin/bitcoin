#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test how invalid blocks will influence chain behavior, in particular
   the acceptance of future blocks and / or headers building on an invalid chain
   For simplicity, submitheader / submitblock rpcs will be used in this test. The
   behavior will be the same if blocks / headers are received through the p2p network.
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.script import CScript
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class InvalidChainTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        tip = int(node.getbestblockhash(), 16)
        start_height = self.nodes[0].getblockcount()

        self.log.info("Test chain with an invalid block (found invalid during connect)")
        # Create invalid block (too high coinbase)
        block_time = node.getblock(node.getbestblockhash())['time'] + 1
        invalid_block = create_block(tip, create_coinbase(start_height + 1, nValue=100), block_time)
        invalid_block.solve()
        # Submit the valid header of the invalid block and a header that builds on it
        node.submitheader(invalid_block.serialize().hex())
        block_time += 1
        block2a = create_block(invalid_block.sha256, create_coinbase(start_height + 2), block_time)
        block2a.solve()
        node.submitheader(block2a.serialize().hex())
        # Submit the invalid block
        assert_equal(node.submitblock(invalid_block.serialize().hex()), "bad-cb-amount")
        # Submit a block building directly on the invalid block
        block_time += 1
        block2b = create_block(invalid_block.sha256, create_coinbase(start_height + 2), block_time)
        block2b.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(block2b.serialize().hex()))
        # Submit a block building indirectly on the invalid block
        block_time += 1
        block3 = create_block(block2a.sha256, create_coinbase(start_height + 3), block_time)
        block3.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(block3.serialize().hex()))

        self.log.info("Test chain with an invalid block (found invalid during acceptance)")
        # Create invalid block (bad coinbase height)
        block_time += 1
        invalid_block = create_block(tip, create_coinbase(start_height + 100), block_time)
        invalid_block.solve()
        # Submit the valid header of the invalid block and a header that builds on it
        node.submitheader(invalid_block.serialize().hex())
        block_time += 1
        block2a = create_block(invalid_block.sha256, create_coinbase(start_height + 2), block_time)
        block2a.solve()
        node.submitheader(block2a.serialize().hex())
        # Submit the invalid block
        assert_equal(node.submitblock(invalid_block.serialize().hex()), "bad-cb-height")
        # Submit a block building directly on the invalid block
        block_time += 1
        block2b = create_block(invalid_block.sha256, create_coinbase(start_height + 2), block_time)
        block2b.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(block2b.serialize().hex()))
        # Submit a block building indirectly on the invalid block
        block_time += 1
        block3 = create_block(block2a.sha256, create_coinbase(start_height + 3), block_time)
        block3.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(block3.serialize().hex()))

        self.log.info("Test chain with an invalid block (found invalid before acceptance)")
        # Create invalid block (too large)
        block_time += 1
        invalid_block = create_block(tip, create_coinbase(start_height + 1, extra_output_script=CScript([b'\x00' * 1000000])), block_time)
        invalid_block.solve()
        # Submit the valid header of the invalid block and a header that builds on it
        node.submitheader(invalid_block.serialize().hex())
        block_time += 1
        block2a = create_block(invalid_block.sha256, create_coinbase(start_height + 2), block_time)
        block2a.solve()
        node.submitheader(block2a.serialize().hex())
        # Submit a block building directly on the invalid block
        block_time += 1
        block2b = create_block(invalid_block.sha256, create_coinbase(start_height + 2), block_time)
        block2b.solve()
        node.submitheader(block2b.serialize().hex())  # No error, we don't mark the block permanently as invalid.
        # Submit a block building indirectly on the invalid block
        assert_equal(node.submitblock(invalid_block.serialize().hex()), "bad-blk-length")
        block_time += 1
        block3 = create_block(block2a.sha256, create_coinbase(start_height + 3), block_time)
        block3.solve()
        node.submitheader(block3.serialize().hex())  # No error, we don't mark the block permanently as invalid.


if __name__ == '__main__':
    InvalidChainTest().main()
