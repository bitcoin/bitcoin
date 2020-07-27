#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the zapwallettxes functionality.

- start two bitcoind nodes
- create two transactions on node 0 - one is confirmed and one is unconfirmed.
- restart node 0 and verify that both the confirmed and the unconfirmed
  transactions are still available.
- restart node 0 with zapwallettxes and persistmempool, and verify that both
  the confirmed and the unconfirmed transactions are still available.
- restart node 0 with just zapwallettxes and verify that the confirmed
  transactions are still available, but that the unconfirmed transaction has
  been zapped.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class ZapWalletTXesTest (BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(101)
        self.sync_all()

        # Make a wallet that we will do the zapping on
        self.nodes[0].createwallet(wallet_name="zaptx")
        zaptx = self.nodes[0].get_wallet_rpc("zaptx")
        default = self.nodes[0].get_wallet_rpc("")

        # This transaction will be confirmed
        txid1 = default.sendtoaddress(zaptx.getnewaddress(), 10)

        self.nodes[0].generate(1)
        self.sync_all()

        # This transaction will not be confirmed
        txid2 = default.sendtoaddress(zaptx.getnewaddress(), 20)

        # Confirmed and unconfirmed transactions are the only transactions in the wallet.
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)
        assert_equal(zaptx.gettransaction(txid2)['txid'], txid2)
        assert_equal(len(zaptx.listtransactions()), 2)

        # Zap with no scans, wallet should not have any txs
        zaptx.zapwallettxes(keep_metadata=False, rescan=False, scan_mempool=False)
        assert_equal(len(zaptx.listtransactions()), 0)

        # Rescanning blockchain should give us the confirmed tx
        zaptx.rescanblockchain(0)
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)

        # Unload and reload wallet to get the mempool rescan
        zaptx.unloadwallet()
        self.nodes[0].loadwallet("zaptx")

        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)
        assert_equal(zaptx.gettransaction(txid2)['txid'], txid2)

        # Zap with blockchain and mempool rescan
        zaptx.zapwallettxes(rescan=True, scan_mempool=True)

        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)
        assert_equal(zaptx.gettransaction(txid2)['txid'], txid2)

        # Zap with no args (just blockchain rescan)
        zaptx.zapwallettxes()

        # tx1 is still be available because it was confirmed
        assert_equal(zaptx.gettransaction(txid1)['txid'], txid1)

        # This will raise an exception because the unconfirmed transaction has been zapped
        assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', zaptx.gettransaction, txid2)

if __name__ == '__main__':
    ZapWalletTXesTest().main()
