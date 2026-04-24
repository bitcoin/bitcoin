#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that inaccessible block files are properly handled through P2P paths."""

import contextlib
import os
import platform
import re
import stat

from test_framework.blocktools import (
    create_block,
    create_empty_fork,
)
from test_framework.messages import (
    BlockTransactionsRequest,
    CInv,
    MSG_BLOCK,
    MSG_WITNESS_FLAG,
    msg_block,
    msg_getblocktxn,
    msg_getdata,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
)


@contextlib.contextmanager
def simulate_io_error(blocks_path):
    # Make block files inaccessible by renaming the blocks directory
    blocks_bak = blocks_path.parent / "blocks_bak"
    parent_dir = blocks_path.parent
    old_mode = stat.S_IMODE(os.stat(parent_dir).st_mode)

    os.rename(blocks_path, blocks_bak)
    os.chmod(parent_dir, 0o500)  # Prevent re-creation of blocks/

    try:
        yield
    finally:
        os.chmod(parent_dir, old_mode)
        os.rename(blocks_bak, blocks_path)

class P2PBlockIOErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-fastprune']]

    def test_block_accept_on_broken_fs(self):
        self.log.info("Test block accept on inaccessible filesystem")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())
        block = create_block(tmpl=node.getblocktemplate({"rules": ["segwit"]}))
        block.solve()

        with simulate_io_error(node.blocks_path):
            with node.assert_debug_log(expected_msgs=["AcceptBlock FAILED (AcceptBlock: Failed to find position to write new block to disk)"]):
                peer.send_without_ping(msg_block(block))
                peer.wait_for_disconnect()

        node.wait_until_stopped(
            expect_error=True,
            expected_stderr=re.compile(r"fatal internal error"),
        )

        # Restart node for next test
        self.start_node(0)

    def test_block_connect_on_broken_fs(self):
        self.log.info("Test block connect on inaccessible filesystem")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())
        blocks = create_empty_fork(node, fork_length=2)
        large_block_hex = self.generatetodescriptor(node, 1, f"raw({'55'*100_000})")[0]
        peer.send_and_ping(msg_block(blocks[0]))
        # Remove access to the fork block only.
        (node.blocks_path / 'blk00002.dat').unlink()
        assert_raises_rpc_error(-1, 'Block not found on disk', node.getblock, blocks[0].hash_hex)
        node.getblock(large_block_hex) # can still access the tip

        with node.assert_debug_log(expected_msgs=["ActivateBestChain failed (Failed to read block.)"]):
            peer.send_without_ping(msg_block(blocks[1]))
            peer.wait_for_disconnect()

        node.wait_until_stopped(
            expect_error=True,
            expected_stderr=re.compile(r"fatal internal error"),
        )

        # Restart node for next test
        self.start_node(0, extra_args=['-reindex'])

    def test_getdata_on_broken_fs(self):
        """The node should not swallow GETDATA requests during I/O issues"""
        self.log.info("Test GETDATA on inaccessible filesystem")
        node = self.nodes[0]
        self.generate(node, 6)

        peer = node.add_p2p_connection(P2PInterface())
        block_hash = node.getblockhash(3)  # Not in recent cache
        with simulate_io_error(node.blocks_path):
            # Request block and expect disconnection.
            with node.assert_debug_log(expected_msgs=["Cannot load block from disk"]):
                peer.send_without_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, int(block_hash, 16))]))
                peer.wait_for_disconnect()

            # Currently, the node keeps running
            node.getblockcount()

    def test_getblocktxn_fatal_on_block_io_error(self):
        """GETBLOCKTXN block read failures should trigger a fatal error.

        Unlike GETDATA, this path operates on recent blocks that must be
        available for normal operation (reorgs). A failure here is
        unrecoverable and must not be swallowed by the P2P message handling
        general try-catch.
        """
        self.log.info("Test GETBLOCKTXN triggers fatal error on inaccessible filesystem")
        node = self.nodes[0]

        # Generate block so that current tip is evicted from the cache.
        self.generate(node, 1)
        block_hash = node.getblockhash(node.getblockcount() - 1)

        peer = node.add_p2p_connection(P2PInterface())
        with simulate_io_error(node.blocks_path):
            gbtn = msg_getblocktxn()
            gbtn.block_txn_request = BlockTransactionsRequest(blockhash=int(block_hash, 16), indexes=[0])

            # Request block and expect fatal error + disconnect.
            with node.assert_debug_log(expected_msgs=["Unable to open file"]):
                peer.send_without_ping(gbtn)
                peer.wait_for_disconnect()

            node.wait_until_stopped(
                expect_error=True,
                expected_stderr=re.compile(r"Failed to read block during GETBLOCKTXN"),
            )

        # Restart node for next test
        self.start_node(0)

    def run_test(self):
        # On Windows, open files (like the DB) are locked and prevent renaming
        # their parent directory, so we can't trigger the expected filesystem error.
        # Also, when running as root, permission changes don't restrict access, so the
        # test condition can't be enforced either.
        if platform.system() == 'Windows' or os.geteuid() == 0:
            self.log.warning("Skipping test: unable to enforce dir permissions")
            return

        self.test_block_accept_on_broken_fs()
        self.test_block_connect_on_broken_fs()
        self.test_getdata_on_broken_fs()
        self.test_getblocktxn_fatal_on_block_io_error()


if __name__ == '__main__':
    P2PBlockIOErrorTest(__file__).main()
