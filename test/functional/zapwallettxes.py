#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the zapwallettxes functionality.

- start two bitcoind nodes
- create two transactions on node 0 - one is confirmed and one is unconfirmed.
- restart node 0 and verify that both the confirmed and the unconfirmed
  transactions are still available.
- restart node 0 with zapwallettxes and persistmempool, and verify that both
  the confirmed and the unconfirmed transactions are still available.
- restart node 0 with just zapwallettxed and verify that the confirmed
  transactions are still available, but that the unconfirmed transaction has
  been zapped.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_equal,
                                 assert_raises_jsonrpc,
                                 )

class ZapWalletTXesTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[1].generate(100)
        self.sync_all()

        # This transaction will be confirmed
        txid1 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 10)

        self.nodes[0].generate(1)
        self.sync_all()

        # This transaction will not be confirmed
        txid2 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 20)

        # Confirmed and unconfirmed transactions are now in the wallet.
        assert_equal(self.nodes[0].gettransaction(txid1)['txid'], txid1)
        assert_equal(self.nodes[0].gettransaction(txid2)['txid'], txid2)

        # Stop-start node0. Both confirmed and unconfirmed transactions remain in the wallet.
        self.stop_node(0)
        self.nodes[0] = self.start_node(0, self.options.tmpdir)

        assert_equal(self.nodes[0].gettransaction(txid1)['txid'], txid1)
        assert_equal(self.nodes[0].gettransaction(txid2)['txid'], txid2)

        # Stop node0 and restart with zapwallettxes and persistmempool. The unconfirmed
        # transaction is zapped from the wallet, but is re-added when the mempool is reloaded.
        self.stop_node(0)
        self.nodes[0] = self.start_node(0, self.options.tmpdir, ["-persistmempool=1", "-zapwallettxes=2"])

        assert_equal(self.nodes[0].gettransaction(txid1)['txid'], txid1)
        assert_equal(self.nodes[0].gettransaction(txid2)['txid'], txid2)

        # Stop node0 and restart with zapwallettxes, but not persistmempool.
        # The unconfirmed transaction is zapped and is no longer in the wallet.
        self.stop_node(0)
        self.nodes[0] = self.start_node(0, self.options.tmpdir, ["-zapwallettxes=2"])

        # tx1 is still be available because it was confirmed
        assert_equal(self.nodes[0].gettransaction(txid1)['txid'], txid1)

        # This will raise an exception because the unconfirmed transaction has been zapped
        assert_raises_jsonrpc(-5, 'Invalid or non-wallet transaction id', self.nodes[0].gettransaction, txid2)

if __name__ == '__main__':
    ZapWalletTXesTest().main()
