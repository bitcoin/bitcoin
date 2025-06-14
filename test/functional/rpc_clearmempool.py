#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test the clearmempool RPC."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet

class RPCClearMempoolTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        # Check that we're in regtest mode
        assert_equal(node.getblockchaininfo()['chain'], 'regtest')

        self.log.info("Generate blocks to create mature coinbase outputs")
        # Generate 101 blocks to get mature coinbase outputs
        self.wallet.generate(101, called_by_framework=True)
        self.sync_all()

        self.log.info("Create and send a transaction")
        # Create and send a transaction
        tx = self.wallet.send_self_transfer(from_node=node)
        assert_equal(len(node.getrawmempool()), 1)

        self.log.info("Clear the mempool")
        # Clear the mempool
        node.clearmempool()
        assert_equal(len(node.getrawmempool()), 0)


if __name__ == '__main__':
    RPCClearMempoolTest(__file__).main() 