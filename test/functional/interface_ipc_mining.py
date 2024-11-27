#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-cli"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

import subprocess
import tempfile

class TestBitcoinMine(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_multiprocess()

    def setup_nodes(self):
        # Always run multiprocess binaries
        self.options.bitcoind = self.options.bitcoin_node

        # Work around default CI path exceeding maximum socket path length
        if len(self.options.tmpdir + "/node0/regtest/node.sock") < 108:
            self.extra_args = [["-ipcbind=unix"]]
            self.mine_args = []
        else:
            sock_path = tempfile.mktemp()
            self.extra_args = [[f"-ipcbind=unix:{sock_path}"]]
            self.mine_args = [f"-ipcconnect=unix:{sock_path}"]
        super().setup_nodes()

    def run_test(self):
        args = [self.options.bitcoin_mine, f"-datadir={self.nodes[0].datadir_path}"] + self.mine_args
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=True)
        assert_equal(result.stdout, "Connected to bitcoin-node\nTip hash is 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206.\n")

if __name__ == '__main__':
    TestBitcoinMine(__file__).main()
