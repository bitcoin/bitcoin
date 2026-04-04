#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that inaccessible block files are properly handled through P2P paths."""

import contextlib
import os
import re
import stat

from test_framework.messages import (
    BlockTransactionsRequest,
    CInv,
    MSG_BLOCK,
    MSG_WITNESS_FLAG,
    msg_getblocktxn,
    msg_getdata,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import can_change_perms


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
        if blocks_bak.exists() and not blocks_path.exists():
            os.rename(blocks_bak, blocks_path)

class P2PBlockIOErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_getdata_on_broken_fs(self):
        """The node should not swallow GETDATA requests during I/O issues"""
        self.log.info("Test GETDATA on inaccessible filesystem")
        node = self.nodes[0]
        self.generate(node, 6, sync_fun=self.no_op)

        peer = node.add_p2p_connection(P2PInterface())
        block_hash = node.getblockhash(3)  # Not in recent cache
        with simulate_io_error(node.blocks_path):
            # Request block and expect disconnection. The node must not swallow the exception and do nothing.
            with node.assert_debug_log(expected_msgs=["Cannot load block from disk"],
                                       unexpected_msgs=["[ProcessMessages] [net] ProcessMessages(getdata, 37 bytes): Exception 'filesystem error: in create_directories"],
                                       timeout=30):
                peer.send_without_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, int(block_hash, 16))]))
                peer.wait_for_disconnect()

            # Currently, the node keeps running.
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

        # Generate two blocks so that the first one is evicted from the
        # m_most_recent_block cache.
        self.generate(node, 1, sync_fun=self.no_op)
        block_hash = node.getblockhash(node.getblockcount() - 1)  # Not in recent cache

        peer = node.add_p2p_connection(P2PInterface())
        with simulate_io_error(node.blocks_path):
            gbtn = msg_getblocktxn()
            gbtn.block_txn_request = BlockTransactionsRequest(blockhash=int(block_hash, 16), indexes=[0])

            # Request block and expect fatal error. The node must not swallow the exception and do nothing.
            with node.assert_debug_log(expected_msgs=["Unable to open file"],
                                       unexpected_msgs=["[ProcessMessages] [net] ProcessMessages(getblocktxn, 34 bytes): Exception 'filesystem error: in create_directories"],
                                       timeout=10):
                peer.send_without_ping(gbtn)

            node.wait_until_stopped(
                expect_error=True,
                expected_stderr=re.compile(r"fatal internal error"),
                timeout=30,
            )

    def run_test(self):
        if os.geteuid() == 0 or not can_change_perms(self.nodes[0].blocks_path):
            self.log.warning("Skipping test: unable to enforce read-only permissions")
            return

        self.test_getdata_on_broken_fs()
        self.test_getblocktxn_fatal_on_block_io_error()


if __name__ == '__main__':
    P2PBlockIOErrorTest(__file__).main()
