#!/usr/bin/env python3
# Copyright (c) 2014-2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests for orphan related RPCs."""

from test_framework.mempool_util import (
    tx_in_orphanage,
)
from test_framework.messages import (
    CInv,
    msg_inv,
    msg_tx,
    MSG_WTX,
)
from test_framework.p2p import P2PInterface
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet


class OrphanRPCsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.test_orphan_activity()
        self.test_orphan_details()
        self.test_misc()

    def test_orphan_activity(self):
        self.log.info("Check that orphaned transactions are returned with getorphantxs")
        node = self.nodes[0]

        self.log.info("Create two 1P1C packages, but only broadcast the children")
        tx_parent_1 = self.wallet.create_self_transfer()
        tx_child_1 = self.wallet.create_self_transfer(utxo_to_spend=tx_parent_1["new_utxo"])
        tx_parent_2 = self.wallet.create_self_transfer()
        tx_child_2 = self.wallet.create_self_transfer(utxo_to_spend=tx_parent_2["new_utxo"])
        peer = node.add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_tx(tx_child_1["tx"]))
        peer.send_and_ping(msg_tx(tx_child_2["tx"]))

        self.log.info("Check that neither parent is in the mempool")
        assert_equal(node.getmempoolinfo()["size"], 0)

        orphanage = node.getorphantxs(verbosity=0)
        self.log.info("Check the size of the orphanage")
        assert_equal(len(orphanage), 2)
        self.log.info("Check that undefined verbosity is disallowed")
        assert_raises_rpc_error(-8, "Invalid verbosity value -1", node.getorphantxs, verbosity=-1)
        assert_raises_rpc_error(-8, "Invalid verbosity value 3", node.getorphantxs, verbosity=3)
        self.log.info("Check that both children are in the orphanage")
        assert tx_in_orphanage(node, tx_child_1["tx"])
        assert tx_in_orphanage(node, tx_child_2["tx"])

        self.log.info("Broadcast parent 1")
        peer.send_and_ping(msg_tx(tx_parent_1["tx"]))
        self.log.info("Check that parent 1 and child 1 are in the mempool")
        raw_mempool = node.getrawmempool()
        assert_equal(len(raw_mempool), 2)
        assert tx_parent_1["txid"] in raw_mempool
        assert tx_child_1["txid"] in raw_mempool

        self.log.info("Check that orphanage only contains child 2")
        orphanage = node.getorphantxs()
        assert_equal(len(orphanage), 1)
        assert tx_in_orphanage(node, tx_child_2["tx"])

        peer.send_and_ping(msg_tx(tx_parent_2["tx"]))
        self.log.info("Check that all parents and children are now in the mempool")
        raw_mempool = node.getrawmempool()
        assert_equal(len(raw_mempool), 4)
        assert tx_parent_1["txid"] in raw_mempool
        assert tx_child_1["txid"] in raw_mempool
        assert tx_parent_2["txid"] in raw_mempool
        assert tx_child_2["txid"] in raw_mempool
        self.log.info("Check that the orphanage is empty")
        assert_equal(len(node.getorphantxs()), 0)

        self.log.info("Confirm the transactions (clears mempool)")
        self.generate(node, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

    def test_orphan_details(self):
        self.log.info("Check the transaction details returned from getorphantxs")
        node = self.nodes[0]

        self.log.info("Create two orphans, from different peers")
        tx_parent_1 = self.wallet.create_self_transfer()
        tx_child_1 = self.wallet.create_self_transfer(utxo_to_spend=tx_parent_1["new_utxo"])
        tx_parent_2 = self.wallet.create_self_transfer()
        tx_child_2 = self.wallet.create_self_transfer(utxo_to_spend=tx_parent_2["new_utxo"])
        peer_1 = node.add_p2p_connection(P2PInterface())
        peer_2 = node.add_p2p_connection(P2PInterface())
        peer_1.send_and_ping(msg_tx(tx_child_1["tx"]))
        peer_2.send_and_ping(msg_tx(tx_child_2["tx"]))

        orphanage = node.getorphantxs(verbosity=2)
        assert tx_in_orphanage(node, tx_child_1["tx"])
        assert tx_in_orphanage(node, tx_child_2["tx"])

        self.log.info("Check that orphan 1 and 2 were from different peers")
        assert_not_equal(orphanage[0]["from"][0], orphanage[1]["from"][0])
        peer_ids = [orphanage[0]["from"][0], orphanage[1]["from"][0]]

        self.log.info("Unorphan child 2")
        peer_2.send_and_ping(msg_tx(tx_parent_2["tx"]))
        assert not tx_in_orphanage(node, tx_child_2["tx"])

        self.log.info("Check that additional announcers are reflected in RPC result")
        peer_2.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=int(tx_child_1["wtxid"], 16))]))

        orphanage = node.getorphantxs(verbosity=2)
        assert_equal(set(orphanage[0]["from"]), set(peer_ids))

        self.log.info("Checking orphan details")
        assert_equal(len(node.getorphantxs()), 1)
        orphan_1 = orphanage[0]
        self.orphan_details_match(orphan_1, tx_child_1, verbosity=1)

        self.log.info("Checking orphan details (verbosity 2)")
        orphanage = node.getorphantxs(verbosity=2)
        orphan_1 = orphanage[0]
        self.orphan_details_match(orphan_1, tx_child_1, verbosity=2)

    def orphan_details_match(self, orphan, tx, verbosity):
        self.log.info("Check txid/wtxid of orphan")
        assert_equal(orphan["txid"], tx["txid"])
        assert_equal(orphan["wtxid"], tx["wtxid"])

        self.log.info("Check the sizes of orphan")
        assert_equal(orphan["bytes"], len(tx["tx"].serialize()))
        assert_equal(orphan["vsize"], tx["tx"].get_vsize())
        assert_equal(orphan["weight"], tx["tx"].get_weight())

        if verbosity == 2:
            self.log.info("Check the transaction hex of orphan")
            assert_equal(orphan["hex"], tx["hex"])

    def test_misc(self):
        node = self.nodes[0]
        assert_raises_rpc_error(-3, "Verbosity was boolean but only integer allowed", node.getorphantxs, verbosity=True)
        assert_raises_rpc_error(-3, "Verbosity was boolean but only integer allowed", node.getorphantxs, verbosity=False)
        help_output = node.help()
        self.log.info("Check that getorphantxs is a hidden RPC")
        assert "getorphantxs" not in help_output
        assert "unknown command: getorphantxs" not in node.help("getorphantxs")


if __name__ == '__main__':
    OrphanRPCsTest(__file__).main()
