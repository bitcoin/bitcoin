#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that wallet/RPC transactions submitted during or after restart while
already in mempool (e.g. from mempool.dat) are added to unbroadcast set
and properly delivered to peers on reconnection."""

from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

MAX_INITIAL_BROADCAST_DELAY = 15 * 60  # 15 minutes in seconds

class WalletRestartMempoolUnbroadcastTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        self.generate(node0, 101)

        self.log.info("Submit transaction to node0 while disconnected from peers")
        self.disconnect_nodes(0, 1)

        # Generate a wallet transaction on node0
        addr = node0.getnewaddress()
        wallet_txid = node0.sendtoaddress(addr, 0.0001)

        # Confirm transaction is in mempool and marked unbroadcast
        assert wallet_txid in node0.getrawmempool()
        assert node0.getmempoolentry(wallet_txid)['unbroadcast']

        # Connect peer, fast-forward to trigger initial broadcast and simulate getdata which removes from unbroadcast
        conn = node0.add_p2p_connection(P2PTxInvStore())
        node0.mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        conn.wait_for_broadcast([wallet_txid])
        # After peer received tx via getdata, unbroadcast is false
        assert not node0.getmempoolentry(wallet_txid)['unbroadcast']

        node0.disconnect_p2ps()

        self.log.info("Restart node0 with persisted mempool (tx loaded from mempool.dat without unbroadcast set)")
        self.restart_node(0)
        assert wallet_txid in node0.getrawmempool()
        # Loaded from mempool.dat as regular entry:
        assert not node0.getmempoolentry(wallet_txid)['unbroadcast']

        self.log.info("Attempt to broadcast/send the transaction again while disconnected")
        # In Bitcoin Core #35589, sending an existing transaction via RPC send/sendrawtransaction/sendtoaddress
        # did NOT add it to the unbroadcast set if it was already in the mempool.
        raw_hex = node0.getrawtransaction(wallet_txid)
        node0.sendrawtransaction(raw_hex)

        # INVARIANT: Transaction MUST be tracked in the unbroadcast set so that ReattemptInitialBroadcast
        # will deliver it to newly connected peers.
        assert_equal(node0.getmempoolentry(wallet_txid)['unbroadcast'], True)

        self.log.info("Connect node1 and fast-forward scheduler to verify delivery")
        self.connect_nodes(0, 1)
        node0.mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        self.sync_mempools(timeout=30)
        assert wallet_txid in node1.getrawmempool()
        assert not node0.getmempoolentry(wallet_txid)['unbroadcast']


if __name__ == '__main__':
    WalletRestartMempoolUnbroadcastTest(__file__).main()
