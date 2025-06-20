#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test support for XORed block data and undo files (`-blocksxor` option)."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import (
    ErrorMatch,
    NULL_BLK_XOR_KEY,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    util_xor,
)
from test_framework.wallet import MiniWallet


class BlocksXORTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            '-blocksxor=1',
            '-fastprune=1',             # use smaller block files
        ]]

    def run_test(self):
        self.log.info("Mine some blocks, to create multiple blk*.dat/rev*.dat files")
        node = self.nodes[0]
        wallet = MiniWallet(node)
        for _ in range(5):
            wallet.send_self_transfer(from_node=node, target_vsize=20000)
            self.generate(wallet, 1)

        block_files = list(node.blocks_path.glob('blk[0-9][0-9][0-9][0-9][0-9].dat'))
        undo_files  = list(node.blocks_path.glob('rev[0-9][0-9][0-9][0-9][0-9].dat'))
        assert_equal(len(block_files), len(undo_files))
        assert_greater_than(len(block_files), 1)  # we want at least one full block file

        self.log.info("Shut down node and un-XOR block/undo files manually")
        self.stop_node(0)
        xor_key = node.read_xor_key()
        for data_file in sorted(block_files + undo_files):
            self.log.debug(f"Rewriting file {data_file}...")
            with open(data_file, 'rb+') as f:
                xored_data = f.read()
                f.seek(0)
                f.write(util_xor(xored_data, xor_key, offset=0))

        self.log.info("Check that restarting with 'blocksxor=0' fails if XOR key is present")
        node.assert_start_raises_init_error(['-blocksxor=0'],
            'The blocksdir XOR-key can not be disabled when a random key was already stored!',
            match=ErrorMatch.PARTIAL_REGEX)

        self.log.info("Delete XOR key, restart node with '-blocksxor=0', check blk*.dat/rev*.dat file integrity")
        node.blocks_key_path.unlink()
        self.start_node(0, extra_args=['-blocksxor=0'])
        # checklevel=2 -> verify block validity + undo data
        # nblocks=0    -> verify all blocks
        node.verifychain(checklevel=2, nblocks=0)
        self.log.info("Check that blocks XOR key is recreated")
        assert_equal(node.read_xor_key(), NULL_BLK_XOR_KEY)


if __name__ == '__main__':
    BlocksXORTest(__file__).main()
