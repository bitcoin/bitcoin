#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test token balance after reorg."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_equal, connect_nodes)

class OmniReorgTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("check token balance after reorg")

        # Get address for mining and token issuance
        token_address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(101, token_address)

        # Create test token
        creation_tx = self.nodes[0].omni_sendissuancefixed(token_address, 2, 2, 0, "", "", "TESTTOKEN", "", "", "100000")
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_all()

        # Send token
        property_id = self.nodes[0].omni_gettransaction(creation_tx)["propertyid"]
        destination = self.nodes[1].getnewaddress()
        self.nodes[0].omni_send(token_address, destination, property_id, "1")

        # Stop second node
        self.stop_node(1)

        # Mine block at height 103 on first node
        self.nodes[0].generatetoaddress(1, token_address)

        # Stop first node and start second node
        self.stop_node(0)
        self.start_node(1)

        # Mine block at height 103 on second node
        self.nodes[1].generatetoaddress(1, destination)
        self.start_node(0)

        # Reconnect nodes and make sure they're fully sycnced
        connect_nodes(self.nodes[0], 1)

        # Make sure nodes are at the expected height and hashes differ
        assert_equal(self.nodes[0].getblockcount(), 103)
        assert_equal(self.nodes[1].getblockcount(), 103)
        if self.nodes[0].getblockhash(103) == self.nodes[1].getblockhash(103):
            raise AssertionError("reorg test expects block hash to differ between nodes at the same height")

        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_all()

        # Make sure nodes are at the expected height and hashes now the same
        assert_equal(self.nodes[0].getblockcount(), 104)
        assert_equal(self.nodes[1].getblockcount(), 104)
        assert_equal(self.nodes[0].getblockhash(104), self.nodes[1].getblockhash(104))

        # We now expect the token balances to be the same in both nodes after the reorg
        assert_equal(str(self.nodes[0].omni_getbalance(destination, property_id)["balance"]), "1.00000000")
        assert_equal(str(self.nodes[1].omni_getbalance(destination, property_id)["balance"]), "1.00000000")

if __name__ == '__main__':
    OmniReorgTest().main()
