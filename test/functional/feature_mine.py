#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-mine"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

import subprocess

class TestBitcoinMine(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()

    def run_test(self):
        node = self.nodes[0]
        args = [self.binary_paths.bitcoinmine, f"-datadir={node.datadir_path}", f"-ipcconnect=unix:{node.ipc_socket_path}"]
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=True)
        assert_equal(result.stdout, "Connected to bitcoin-node\nTip hash is 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206.\n")

if __name__ == '__main__':
    TestBitcoinMine(__file__).main()
