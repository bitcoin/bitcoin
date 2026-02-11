#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test corruption behavior on database read paths."""
import contextlib
import http.client
import subprocess

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework

# Exceptions expected when the node aborts mid-RPC-request.
RPC_CONN_LOST = (
    subprocess.CalledProcessError,
    http.client.CannotSendRequest,
    http.client.RemoteDisconnected,
    ConnectionResetError,
)


class DBReadErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        # Separate nodes avoid having to rebuild datadirs between corruption cases.
        self.num_nodes = 3

    def setup_network(self):
        self.setup_nodes()

    @staticmethod
    def for_txids(node, callback):
        for height in range(1, node.getblockcount() + 1):
            txid = node.getblock(node.getblockhash(height))["tx"][0]
            callback(txid)

    @staticmethod
    def corrupt_leveldb(node, leveldb_dir):
        db_path = node.chain_path / leveldb_dir
        for table_path in sorted(db_path.glob("*.ldb")) + sorted(db_path.glob("*.sst")):
            with open(table_path, "r+b") as f:
                file_size = table_path.stat().st_size
                for offset in (file_size // 3, file_size // 2, (file_size * 2) // 3):
                    f.seek(offset)
                    f.write(b"\xff")

    @staticmethod
    def assert_shutdown_on_read_error(node, rpc_fn, expected_msgs=None):
        with node.assert_debug_log(expected_msgs) if expected_msgs is not None else contextlib.nullcontext():
            try:
                rpc_fn()
            except RPC_CONN_LOST:
                pass

        node.wait_until(lambda: node.process.poll(), timeout=60)
        assert node.is_node_stopped(
            expected_stderr="Error: Cannot read from database, shutting down.",
            expected_ret_code=node.process.returncode,
        )

    @staticmethod
    def assert_read_error_without_shutdown(node, rpc_fn):
        try:
            rpc_fn()
        except (JSONRPCException, *RPC_CONN_LOST):
            pass

        assert node.process.poll() is None
        node.getblockcount()

    def test_coinsdb_gettxout_read_error(self):
        node = self.nodes[0]
        # Stop node to clear LevelDB block cache (required for 32-bit where mmap is disabled)
        self.stop_node(0)
        self.corrupt_leveldb(node, "chainstate")
        self.start_node(0, extra_args=["-checkblocks=0", "-checklevel=0"])

        self.assert_shutdown_on_read_error(
            node,
            rpc_fn=lambda: self.for_txids(node, lambda txid: node.gettxout(txid, 0)),
            expected_msgs=["block checksum mismatch"],
        )

    def test_txindex_getrawtransaction_read_error(self):
        node = self.nodes[2]
        # Build txindex with small dbcache, then compact to ensure data lands in table files.
        for _ in range(2):
            self.restart_node(2, extra_args=["-txindex=1", "-dbcache=4", "-forcecompactdb=1"])
            node.wait_until(lambda: node.getindexinfo().get("txindex", {}).get("synced", False))

        self.stop_node(2)
        self.corrupt_leveldb(node, "indexes/txindex")
        self.start_node(2, extra_args=["-txindex=1"])

        self.assert_shutdown_on_read_error(node, rpc_fn=lambda: self.for_txids(node, node.getrawtransaction))

    def test_coinsdb_gettxoutsetinfo_read_error(self):
        node = self.nodes[1]
        # Corrupt after startup to avoid turning this into a DB-open failure path.
        self.corrupt_leveldb(node, "chainstate")

        self.assert_read_error_without_shutdown(node, rpc_fn=lambda: node.gettxoutsetinfo("none"))  # TODO: assert that shutdown after iterator decode errors throw

    def run_test(self):
        self.log.info("coinsdb: corrupt chainstate and trigger read via gettxout")
        self.test_coinsdb_gettxout_read_error()

        self.log.info("coinsdb: corrupt chainstate and trigger read via gettxoutsetinfo")
        self.test_coinsdb_gettxoutsetinfo_read_error()

        self.log.info("txindex: corrupt index DB and trigger read via getrawtransaction")
        self.test_txindex_getrawtransaction_read_error()


if __name__ == '__main__':
    DBReadErrorTest(__file__).main()
