#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test corruption behavior on coin and index point-read paths."""
import http.client
import re
import subprocess

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, sync_txindex


class DBReadErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], ['-txindex=1']]

    def setup_network(self):
        self.setup_nodes()

    @staticmethod
    def corrupt_tables(node, directory):
        db_path = node.chain_path / directory
        table_paths = [*db_path.glob('*.ldb'), *db_path.glob('*.sst')]
        assert table_paths
        for table_path in table_paths:
            with open(table_path, 'r+b') as f:
                file_size = table_path.stat().st_size
                # Hit data blocks in the middle and leave the footer intact, so the database still opens
                for offset in (file_size // 3, file_size // 2, file_size * 2 // 3):
                    f.seek(offset)
                    f.write(b'\xff')

    @staticmethod
    def trigger_read_error(node, read_tx):
        with node.assert_debug_log(['LevelDB read failure']):
            try:
                for height in range(1, node.getblockcount() + 1):
                    read_tx(node.getblock(node.getblockhash(height))['tx'][0])
            except JSONRPCException as e:
                assert_equal(e.error['code'], -1)
                assert 'Fatal LevelDB error' in e.error['message']
            except (subprocess.CalledProcessError, http.client.CannotSendRequest, http.client.RemoteDisconnected, ConnectionError):
                pass  # Aborting may close the RPC connection

    @staticmethod
    def assert_shutdown(node):
        node.wait_until_stopped(
            expected_stderr=re.compile(r'A fatal internal error occurred, see .* for details: Error reading from database, shutting down\.'),
            expected_ret_code=[-6, 3, 0xC0000409],  # Unix, Windows native, Windows cross builds
        )

    def run_test(self):
        self.log.info('Corrupt chainstate and trigger a read through gettxout')
        node = self.nodes[0]
        self.stop_node(0)  # Clear the block cache before corruption, including on 32-bit systems
        self.corrupt_tables(node, 'chainstate')
        self.start_node(0, extra_args=['-checkblocks=0', '-checklevel=0'])
        self.trigger_read_error(node, lambda txid: node.gettxout(txid, 0))
        self.assert_shutdown(node)

        self.log.info('Build txindex and corrupt its tables')
        node = self.nodes[1]
        sync_txindex(self, node)
        self.restart_node(1)  # Log recovery on startup writes the index from the WAL into a .ldb table
        self.stop_node(1)
        self.corrupt_tables(node, 'indexes/txindex')
        self.start_node(1)
        self.trigger_read_error(node, node.getrawtransaction)
        assert node.process.poll() is None  # TODO: Abort instead of continuing after an index point-read error


if __name__ == '__main__':
    DBReadErrorTest(__file__).main()
