#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

class DeprecatedRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], ['-deprecatedrpc=bip125']]

    def run_test(self):
        # This test should be used to verify correct behaviour of deprecated
        # RPC methods with and without the -deprecatedrpc flags. For example:
        #
        # In set_test_params:
        # self.extra_args = [[], ["-deprecatedrpc=generate"]]
        #
        # In run_test:
        # self.log.info("Test generate RPC")
        # assert_raises_rpc_error(-32, 'The wallet generate rpc method is deprecated', self.nodes[0].rpc.generate, 1)
        # self.nodes[1].generate(1)

        self.log.info("Test deprecated fields for RBF signaling")
        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.scan_blocks(start=76, num=1)
        txid = self.wallet.send_self_transfer(from_node=self.nodes[0])["txid"]
        self.sync_all()
        node0_entry = self.nodes[0].getmempoolentry(txid)
        assert "replaceable" in node0_entry
        assert "bip125-replaceable" not in node0_entry
        node1_entry = self.nodes[1].getmempoolentry(txid)
        assert "replaceable" in node1_entry
        assert "bip125-replaceable" in node1_entry
        assert_equal(node1_entry["replaceable"], node1_entry["bip125-replaceable"])


if __name__ == '__main__':
    DeprecatedRpcTest().main()
