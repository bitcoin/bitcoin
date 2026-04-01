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
    msg_getcfilters,
    msg_getdata,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import (
    BitcoinTestFramework,
    SkipTest
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


@contextlib.contextmanager
def simulate_io_error(target_path):
    # Make files under target_path inaccessible.
    # Simulating a detached volume or permission change.
    backup = target_path.with_name(target_path.name + "_bak")
    parent_dir = target_path.parent
    old_mode = stat.S_IMODE(os.stat(parent_dir).st_mode)

    os.rename(target_path, backup)
    os.chmod(parent_dir, 0o500)  # Prevent dir re-creation
    try:
        yield
    finally:
        os.chmod(parent_dir, old_mode)
        os.rename(backup, target_path)

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
        large_block_hash = self.generatetodescriptor(node, 1, f"raw({'55'*70_000})")[0]
        peer.send_and_ping(msg_block(blocks[0]))
        # Remove access to the fork block only.
        (node.blocks_path / 'blk00002.dat').unlink()
        assert_raises_rpc_error(-1, 'Block not found on disk', node.getblock, blocks[0].hash_hex)
        assert_equal(node.getblock(large_block_hash)["hash"], large_block_hash) # can still access the tip

        with node.assert_debug_log(expected_msgs=["ActivateBestChain failed (Failed to read block.)"]):
            peer.send_without_ping(msg_block(blocks[1]))
            peer.wait_for_disconnect()

        node.wait_until_stopped(
            expect_error=True,
            expected_stderr=re.compile(r"fatal internal error"),
        )

        # Restart node for next test. Only the fork block file was deleted,
        # so reindexing the remaining block files must restore the old tip.
        self.start_node(0, extra_args=['-reindex'])
        self.wait_until(lambda: node.getbestblockhash() == large_block_hash)

    def test_getdata_on_broken_fs(self):
        """GETDATA handler must discard the failed request and disconnect during block I/O issues"""
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
        """GETBLOCKTXN block read failures must trigger a fatal shutdown, not an assertion or silent catch.

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

            # Request block and expect crash+disconnect.
            with node.assert_debug_log(expected_msgs=["Unable to open file"]):
                peer.send_without_ping(gbtn)
                peer.wait_for_disconnect()

            node.wait_until_stopped(
                expected_ret_code=-6,
                expect_error=True,
                expected_stderr=re.compile("Assertion"), # assertion crash
            )

        # Restart node for next test
        self.start_node(0)

    def test_getcfilters_on_broken_fs(self):
        """GETCFILTERS must log the filter-index read failure and keep running."""
        self.log.info("Test GETCFILTERS on inaccessible filter index filesystem")
        node = self.nodes[0]
        self.restart_node(0, extra_args=['-blockfilterindex=1', '-peerblockfilters=1'])
        self.generate(node, 5)
        self.wait_until(lambda: all(i["synced"] for i in node.getindexinfo().values()))

        filter_dir = node.blocks_path.parent / "indexes" / "blockfilter" / "basic"
        stop_hash = node.getbestblockhash()

        height = node.getblockcount()
        peer = node.add_p2p_connection(P2PInterface())
        with simulate_io_error(filter_dir):
            with node.assert_debug_log(expected_msgs=["Failed to find block filter in index"]):
                peer.send_and_ping(msg_getcfilters(filter_type=0, start_height=0, stop_hash=int(stop_hash, 16)))
            # Request dropped (no cfilter reply), peer stays connected, node keeps running.
            assert "cfilter" not in peer.last_message
            # And the node keeps running
            assert_equal(node.getblockcount(), height)

    def test_index_read_on_broken_fs(self):
        """txindex and txospenderindex block file reads should handle filesystem errors properly."""
        self.log.info("Test index reads on inaccessible filesystem")
        node = self.nodes[0]
        self.restart_node(0, extra_args=['-txindex=1', '-txospenderindex=1'])

        # Confirm a spend so both indexes have info to retrieve
        wallet = MiniWallet(node)
        self.generate(wallet, 1)
        self.generate(node, 100)  # mature the coinbase
        spent = wallet.get_utxo()
        spend = wallet.send_self_transfer(from_node=node, utxo_to_spend=spent)
        self.generate(node, 1)    # confirm the spend
        self.wait_until(lambda: all(i["synced"] for i in node.getindexinfo().values()))

        height = node.getblockcount()
        with simulate_io_error(node.blocks_path):
            with node.assert_debug_log(expected_msgs=["OpenBlockFile failed"]):
                assert_raises_rpc_error(-5, "No such mempool or blockchain transaction", node.getrawtransaction, spend["txid"])

            with node.assert_debug_log(expected_msgs=["Deserialize or I/O error - cannot open block"]):
                assert_raises_rpc_error(-1, "IO error finding spending tx for outpoint", node.gettxspendingprevout, [{"txid": spent["txid"], "vout": spent["vout"]}])

            # And the node keeps running
            assert_equal(node.getblockcount(), height)

    def run_test(self):
        # On Windows, open files (like the DB) are locked and prevent renaming
        # their parent directory, so we can't trigger the expected filesystem error.
        # Also, when running as root, permission changes don't restrict access, so the
        # test condition can't be enforced either.
        if platform.system() == 'Windows' or os.geteuid() == 0:
            raise SkipTest("Unable to enforce dir permissions")

        self.test_block_accept_on_broken_fs()
        self.test_block_connect_on_broken_fs()
        self.test_getdata_on_broken_fs()
        self.test_getblocktxn_fatal_on_block_io_error()
        self.test_getcfilters_on_broken_fs()
        self.test_index_read_on_broken_fs()


if __name__ == '__main__':
    P2PBlockIOErrorTest(__file__).main()
