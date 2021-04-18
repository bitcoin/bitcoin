#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the mempool ensures transaction delivery by periodically sending
to peers until a GETDATA is received."""

import time

from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_equal,
    create_confirmed_utxos,
)

MAX_INITIAL_BROADCAST_DELAY = 15 * 60 # 15 minutes in seconds

class MempoolUnbroadcastTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_broadcast()
        self.test_txn_removal()

    def test_broadcast(self):
        self.log.info("Test that mempool reattempts delivery of locally submitted transaction")
        node = self.nodes[0]

        min_relay_fee = node.getnetworkinfo()["relayfee"]
        utxos = create_confirmed_utxos(min_relay_fee, node, 10)

        self.disconnect_nodes(0, 1)

        self.log.info("Generate transactions that only node 0 knows about")

        # generate a wallet txn
        addr = node.getnewaddress()
        wallet_tx_hsh = node.sendtoaddress(addr, 0.0001)

        # generate a txn using sendrawtransaction
        us0 = utxos.pop()
        inputs = [{"txid": us0["txid"], "vout": us0["vout"]}]
        outputs = {addr: 0.0001}
        tx = node.createrawtransaction(inputs, outputs)
        node.settxfee(min_relay_fee)
        txF = node.fundrawtransaction(tx)
        txFS = node.signrawtransactionwithwallet(txF["hex"])
        rpc_tx_hsh = node.sendrawtransaction(txFS["hex"])

        # check transactions are in unbroadcast using rpc
        mempoolinfo = self.nodes[0].getmempoolinfo()
        assert_equal(mempoolinfo['unbroadcastcount'], 2)
        mempool = self.nodes[0].getrawmempool(True)
        for tx in mempool:
            assert_equal(mempool[tx]['unbroadcast'], True)

        # check that second node doesn't have these two txns
        mempool = self.nodes[1].getrawmempool()
        assert rpc_tx_hsh not in mempool
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
        assert wallet_tx_hsh in mempool

        # check that transactions are no longer in first node's unbroadcast set
        mempool = self.nodes[0].getrawmempool(True)
        for tx in mempool:
            assert_equal(mempool[tx]['unbroadcast'], False)

        self.log.info("Add another connection & ensure transactions aren't broadcast again")

        conn = node.add_p2p_connection(P2PTxInvStore())
        node.mockscheduler(MAX_INITIAL_BROADCAST_DELAY)
        time.sleep(2) # allow sufficient time for possibility of broadcast
        assert_equal(len(conn.get_invs()), 0)

        self.disconnect_nodes(0, 1)
        node.disconnect_p2ps()

    def test_txn_removal(self):
        self.log.info("Test that transactions removed from mempool are removed from unbroadcast set")
        node = self.nodes[0]

        # since the node doesn't have any connections, it will not receive
        # any GETDATAs & thus the transaction will remain in the unbroadcast set.
        addr = node.getnewaddress()
        txhsh = node.sendtoaddress(addr, 0.0001)

        # check transaction was removed from unbroadcast set due to presence in
        # a block
        removal_reason = "Removed {} from set of unbroadcast txns before confirmation that txn was sent out".format(txhsh)
        with node.assert_debug_log([removal_reason]):
            node.generate(1)

if __name__ == "__main__":
    MempoolUnbroadcastTest().main()
