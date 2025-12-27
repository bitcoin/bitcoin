#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test that coins database read errors trigger a shutdown message."""
import platform

from test_framework.test_framework import BitcoinTestFramework


class CoinsDBReadErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        self.skip_if_no_ipc()

        node = self.nodes[0]
        for ldb_path in (node.chain_path / "chainstate").glob("*.ldb"):
            with open(ldb_path, "r+b") as f:
                f.write(b'\xff')

        with node.assert_debug_log(["block checksum mismatch"]):
            try:
                txid = node.getblock(node.getblockhash(1))["tx"][0]
                node.gettxout(txid, 0)
            except Exception:
                pass

        node.wait_until_stopped(expected_stderr="Error: Cannot read database, shutting down.",
                                expected_ret_code=3 if platform.system() == "Windows" else -6)


if __name__ == '__main__':
    CoinsDBReadErrorTest(__file__).main()
