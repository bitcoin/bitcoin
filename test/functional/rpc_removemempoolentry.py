#!/usr/bin/env python3

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class RemoveMempoolEntryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)

        self.log.debug("Mine a single block to get out of IBD")
        self.nodes[0].generate(1)
        self.sync_all()

        self.log.debug("Send 5 transactions from node0")
        for i in range(5):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), Decimal("10"))
        self.sync_all()

        self.log.debug("Verify that node0 and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

        self.log.debug("Send 1 transaction from node1")
        last_txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), Decimal("10"))

        self.log.debug("Verify that node0 have 6 transactions and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 6)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

        self.log.debug("Send last transaction from node1 mempool")
        self.nodes[0].removemempoolentry(txid=last_txid)

        self.log.debug("Verify that node0 and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)



if __name__ == '__main__':
    RemoveMempoolEntryTest().main()
