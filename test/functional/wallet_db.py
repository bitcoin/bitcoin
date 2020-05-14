#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet database layer & recovery features.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

from pathlib import Path

class WalletDbTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        # Unload default wallet, write corrupt log file, verify fails to reload
        node.unloadwallet("")
        Path(node.datadir, self.chain, "database", "log.0000000001").write_bytes(b'\1'*1024)
        assert_raises_rpc_error(-4, "This error could occur if this wallet was not shutdown cleanly and was last loaded using a build with a newer version of Berkeley DB.", node.loadwallet, "")

if __name__ == '__main__':
    WalletDbTest().main()
