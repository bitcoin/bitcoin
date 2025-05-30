#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the mempool ensures transaction delivery by periodically sending
to peers until a GETDATA is received."""

from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
)
from test_framework.wallet import MiniWallet

MAX_INITIAL_BROADCAST_DELAY = 15 * 60 # 15 minutes in seconds

class MempoolUnbroadcastTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.uses_wallet = None

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.test_broadcast()
        self.test_txn_removal()

    def test_broadcast(self):
        self.log.info("Test that mempool reattempts delivery of locally submitted transaction")
        node = self.nodes[0]

        self.disconnect_nodes(0, 1)

        self.log.info("Generate transactions that only node 0 knows about")

        if self.is_wallet_compiled():
            self.import_deterministic_coinbase_privkeys()
            # generate a wallet txn
            addr = node.getnewaddress()
            wallet_tx_hsh = node.sendtoaddress(addr, 0.0001)

        # generate a txn using sendrawtransaction
        txFS = self.wallet.create_self_transfer()
        rpc_tx_hsh = node.sendrawtransaction(txFS["hex"])

        # check transactions are in unbroadcast using rpc
        mempoolinfo = self.nodes[0].getmempoolinfo()
        unbroadcast_count = 1
        if self.is_wallet_compiled():
            unbroadcast_count += 1
        assert_equal(mempoolinfo['unbroadcastcount'], unbroadcast_count)
        mempool = self.nodes[0].getrawmempool(True)
        for tx in mempool:
            assert_equal(mempool[tx]['unbroadcast'], True)

        # check that second node doesn't have these two txns
        mempool = self.nodes[1].getrawmempool()
        assert rpc_tx_hsh not in mempool
        if self.is_wallet_compiled():
            assert wallet_tx_hsh not in mempool

        # ensure that unbroadcast txs are persisted to mempool.dat
        self.restart_node(0)

        self.log.info("Reconnect nodes & check if they are sent to node 1")
        self.connect_nodes(0, 1)

        # fast forward into the future & ensure that the second node has the txns
        node.mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        self.sync_mempools(timeout=30)
        mempool = self.nodes[1].getrawmempool()
        assert rpc_tx_hsh in mempool
        if self.is_wallet_compiled():
            assert wallet_tx_hsh in mempool

        # check that transactions are no longer in first node's unbroadcast set
        mempool = self.nodes[0].getrawmempool(True)
        for tx in mempool:
            assert_equal(mempool[tx]['unbroadcast'], False)

        self.log.info("Add another connection & ensure transactions aren't broadcast again")

        conn = node.add_p2p_connection(P2PTxInvStore())
        node.mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        # allow sufficient time for possibility of broadcast
        ensure_for(duration=2, f=lambda: len(conn.get_invs()) == 0)

        self.disconnect_nodes(0, 1)
        node.disconnect_p2ps()

        self.log.info("Rebroadcast transaction and ensure it is not added to unbroadcast set when already in mempool")
        rpc_tx_hsh = node.sendrawtransaction(txFS["hex"])
        assert not node.getmempoolentry(rpc_tx_hsh)['unbroadcast']

    def test_txn_removal(self):
        self.log.info("Test that transactions removed from mempool are removed from unbroadcast set")
        node = self.nodes[0]

        # since the node doesn't have any connections, it will not receive
        # any GETDATAs & thus the transaction will remain in the unbroadcast set.
        txhsh = self.wallet.send_self_transfer(from_node=node)["txid"]

        # check transaction was removed from unbroadcast set due to presence in
        # a block
        removal_reason = "Removed {} from set of unbroadcast txns before confirmation that txn was sent out".format(txhsh)
        with node.assert_debug_log([removal_reason]):
            self.generate(node, 1, sync_fun=self.no_op)


if __name__ == "__main__":
    MempoolUnbroadcastTest(__file__).main()
