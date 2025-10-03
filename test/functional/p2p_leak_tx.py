#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction upload"""

from test_framework.messages import msg_getdata, CInv, MSG_TX, MSG_WTX
from test_framework.p2p import p2p_lock, P2PDataStore, P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet

import time

class P2PNode(P2PDataStore):
    def on_inv(self, message):
        pass


class P2PLeakTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.gen_node = self.nodes[0]  # The block and tx generating node
        self.miniwallet = MiniWallet(self.gen_node)
        self.mocktime = int(time.time())

        self.test_tx_in_block()
        self.test_notfound_on_replaced_tx()
        self.test_notfound_on_unannounced_tx()

    def test_tx_in_block(self):
        self.log.info("Check that a transaction in the last block is uploaded (beneficial for compact block relay)")
        self.gen_node.setmocktime(self.mocktime)
        inbound_peer = self.gen_node.add_p2p_connection(P2PNode())

        self.log.debug("Generate transaction and block")
        inbound_peer.last_message.pop("inv", None)

        wtxid = self.miniwallet.send_self_transfer(from_node=self.gen_node)["wtxid"]
        rawmp = self.gen_node.getrawmempool(False, True)
        pi = self.gen_node.getpeerinfo()[0]
        assert_equal(rawmp["mempool_sequence"], 2) # our tx cause mempool activity
        assert_equal(pi["last_inv_sequence"], 1) # that is after the last inv
        assert_equal(pi["inv_to_send"], 1) # and our tx has been queued
        self.mocktime += 120
        self.gen_node.setmocktime(self.mocktime)
        inbound_peer.wait_until(lambda: "inv" in inbound_peer.last_message and inbound_peer.last_message.get("inv").inv[0].hash == int(wtxid, 16))

        rawmp = self.gen_node.getrawmempool(False, True)
        pi = self.gen_node.getpeerinfo()[0]
        assert_equal(rawmp["mempool_sequence"], 2) # no mempool update
        assert_equal(pi["last_inv_sequence"], 2) # announced the current mempool
        assert_equal(pi["inv_to_send"], 0) # nothing left in the queue

        want_tx = msg_getdata(inv=inbound_peer.last_message.get("inv").inv)
        self.generate(self.gen_node, 1)

        self.log.debug("Request transaction")
        inbound_peer.last_message.pop("tx", None)
        inbound_peer.send_and_ping(want_tx)
        assert_equal(inbound_peer.last_message.get("tx").tx.wtxid_hex, wtxid)

    def test_notfound_on_replaced_tx(self):
        self.gen_node.disconnect_p2ps()
        self.gen_node.setmocktime(self.mocktime)
        inbound_peer = self.gen_node.add_p2p_connection(P2PTxInvStore())

        self.log.info("Transaction tx_a is broadcast")
        tx_a = self.miniwallet.send_self_transfer(from_node=self.gen_node)
        self.mocktime += 120
        self.gen_node.setmocktime(self.mocktime)
        inbound_peer.wait_for_broadcast(txns=[tx_a["wtxid"]])

        tx_b = tx_a["tx"]
        tx_b.vout[0].nValue -= 9000
        self.gen_node.sendrawtransaction(tx_b.serialize().hex())
        self.mocktime += 120
        self.gen_node.setmocktime(self.mocktime)
        inbound_peer.wait_until(lambda: "tx" in inbound_peer.last_message and inbound_peer.last_message.get("tx").tx.wtxid_hex == tx_b.wtxid_hex)

        self.log.info("Re-request of tx_a after replacement is answered with notfound")
        req_vec = [
            CInv(t=MSG_TX, h=int(tx_a["txid"], 16)),
            CInv(t=MSG_WTX, h=int(tx_a["wtxid"], 16)),
        ]
        want_tx = msg_getdata()
        want_tx.inv = req_vec
        with p2p_lock:
            inbound_peer.last_message.pop("notfound", None)
            inbound_peer.last_message.pop("tx", None)
        inbound_peer.send_and_ping(want_tx)

        assert_equal(inbound_peer.last_message.get("notfound").vec, req_vec)
        assert "tx" not in inbound_peer.last_message

    def test_notfound_on_unannounced_tx(self):
        self.log.info("Check that we don't leak txs to inbound peers that we haven't yet announced to")
        self.gen_node.disconnect_p2ps()
        inbound_peer = self.gen_node.add_p2p_connection(P2PNode())  # An "attacking" inbound peer

        # Set a mock time so that time does not pass, and gen_node never announces the transaction
        self.gen_node.setmocktime(self.mocktime)
        wtxid = int(self.miniwallet.send_self_transfer(from_node=self.gen_node)["wtxid"], 16)

        want_tx = msg_getdata()
        want_tx.inv.append(CInv(t=MSG_WTX, h=wtxid))
        with p2p_lock:
            inbound_peer.last_message.pop('notfound', None)
        inbound_peer.send_and_ping(want_tx)
        inbound_peer.wait_until(lambda: "notfound" in inbound_peer.last_message)
        with p2p_lock:
            assert_equal(inbound_peer.last_message.get("notfound").vec[0].hash, wtxid)
            inbound_peer.last_message.pop('notfound')

        # Move mocktime forward and wait for the announcement.
        inbound_peer.last_message.pop('inv', None)
        self.mocktime += 120
        self.gen_node.setmocktime(self.mocktime)
        inbound_peer.wait_for_inv([CInv(t=MSG_WTX, h=wtxid)], timeout=120)

        # Send the getdata again, this time the node should send us a TX message.
        inbound_peer.last_message.pop('tx', None)
        inbound_peer.send_and_ping(want_tx)
        self.wait_until(lambda: "tx" in inbound_peer.last_message)
        assert_equal(wtxid, int(inbound_peer.last_message["tx"].tx.wtxid_hex, 16))


if __name__ == '__main__':
    P2PLeakTxTest(__file__).main()
