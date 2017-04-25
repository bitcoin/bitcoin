#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet import RPCs.

Test rescan behavior of importprivkey when aborted. The test ensures that:
1. The abortrescan command indeed stops the rescan process.
2. Subsequent rescan catches the aborted address UTXO
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_equal, get_rpc_proxy)
from decimal import Decimal
import threading # for bg importprivkey
import time      # for sleep

class ImportAbortRescanTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True

    def run_test(self):
        # Generate for BTC
        assert_equal(self.nodes[0].getbalance(), 0)
        assert_equal(self.nodes[1].getbalance(), 0)
        self.nodes[0].generate(300)
        assert_equal(self.nodes[1].getbalance(), 0)
        # Make blocks with spam to cause rescan delay
        for i in range(5):
            for j in range(5):
                self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.1)
            self.nodes[0].generate(10)
        addr = self.nodes[0].getnewaddress()
        privkey = self.nodes[0].dumpprivkey(addr)
        self.nodes[0].sendtoaddress(addr, 0.123)
        self.nodes[0].generate(10) # mature tx
        self.sync_all()

        # Import this address in the background ...
        node1ref = get_rpc_proxy(self.nodes[1].url, 1, timeout=600)
        importthread = threading.Thread(target=node1ref.importprivkey, args=[privkey])
        importthread.start()
        # ... then abort rescan; try a bunch until abortres becomes true,
        # because we will start checking before above thread starts processing
        for i in range(2000):
            time.sleep(0.001)
            abortres = self.nodes[1].abortrescan()
            if abortres: break
        assert abortres # if false, we failed to abort
        # import should die soon
        for i in range(10):
            time.sleep(0.1)
            deadres = not importthread.isAlive()
            if deadres: break

        assert deadres # if false, importthread did not die soon enough
        assert_equal(self.nodes[1].getbalance(), 0.0)

        # Import a different address and let it run
        self.nodes[1].importprivkey(self.nodes[0].dumpprivkey(self.nodes[0].getnewaddress()))
        # Expect original privkey to now also be discovered and added to balance
        assert_equal(self.nodes[1].getbalance(), Decimal("0.123"))

if __name__ == "__main__":
    ImportAbortRescanTest().main()
