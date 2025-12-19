#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test that coins database read errors trigger a shutdown message."""
import http
import subprocess

from test_framework.test_framework import BitcoinTestFramework


class CoinsDBReadErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]

        # Stop node to clear LevelDB block cache (required for 32-bit where mmap is disabled)
        self.stop_node(0)

        for ldb_path in (node.chain_path / "chainstate").glob("*.ldb"):
            with open(ldb_path, "r+b") as f:
                f.seek(ldb_path.stat().st_size // 2)
                f.write(b'\xff')

        self.start_node(0, extra_args=["-checkblocks=0", "-checklevel=0"])

        with node.assert_debug_log(["block checksum mismatch"]):
            try:
                for height in range(1, node.getblockcount() + 1):
                    txid = node.getblock(node.getblockhash(height))["tx"][0]
                    node.gettxout(txid, 0)
            except (subprocess.CalledProcessError,
                    http.client.CannotSendRequest,
                    http.client.RemoteDisconnected,
                    ConnectionResetError):
                pass

        node.wait_until(lambda: node.process.poll())
        assert node.is_node_stopped(expected_stderr="Error: Cannot read from database, shutting down.",
                                    expected_ret_code=node.process.returncode)


if __name__ == '__main__':
    CoinsDBReadErrorTest(__file__).main()
