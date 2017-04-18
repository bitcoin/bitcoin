#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet import RPCs.

Test rescan behavior of importprivkey when aborted. The test ensures that:
1. The abortrescan command indeed stops the rescan process.
2. Subsequent rescan catches the aborted address UTXO.
"""

from decimal import Decimal
import threading # for bg importprivkey
from time import sleep
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_equal, get_rpc_proxy)

class ImportAbortRescanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def run_test(self):
        # Generate for BTC
        self.nodes[0].generate(300)
        self.log.info("Creating blocks with transactions ...")
        # Make blocks with spam to cause rescan delay
        for i in range(200):
            if i % 50 == 0:
                self.log.info("... %2.f%%", 100.0 / 200.0 * i)
            addr = self.nodes[0].getnewaddress()
            for j in range(10):
                self.nodes[0].sendtoaddress(addr, 0.1)
            self.nodes[0].generate(1)
        self.log.info("Sending to shared address ...")
        addr = self.nodes[0].getnewaddress()
        privkey = self.nodes[0].dumpprivkey(addr)
        self.nodes[0].sendtoaddress(addr, 0.123)
        self.nodes[0].generate(10) # mature tx
        self.sync_all()
        balances = [n.getbalance() for n in self.nodes]

        # Import this address in the background ...
        self.log.info("Importing address in background thread ...")
        node1ref = get_rpc_proxy(self.nodes[1].url, 1, timeout=600)
        importthread = threading.Thread(target=node1ref.importprivkey, args=[privkey])
        importthread.start()
        # ... then abort rescan; try a bunch until abortres becomes true,
        # because we will start checking before above thread starts processing
        self.log.info("Attempting to abort scan ...")
        for i in range(2000):
            sleep(0.001)
            abortres = self.nodes[1].abortrescan()
            if abortres:
                break
        assert abortres, "failed to abort rescan within allotted time"
        self.log.info("Waiting for import thread to die ...")
        # import should die soon
        for i in range(10):
            sleep(0.1)
            deadres = not importthread.isAlive()
            if deadres:
                break

        assert deadres, "importthread did not die soon enough"
        # Node 1 should not have gained any balance since the start
        assert_equal(self.nodes[1].getbalance(), balances[1])

        # Import a different address and let it run
        self.nodes[1].importprivkey(self.nodes[0].dumpprivkey(self.nodes[0].getnewaddress()))
        # Expect original privkey to now also be discovered and added to balance
        assert_equal(self.nodes[1].getbalance(), Decimal("0.123") + Decimal(balances[1]))

if __name__ == "__main__":
    ImportAbortRescanTest().main()
