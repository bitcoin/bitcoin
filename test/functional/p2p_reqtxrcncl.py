#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test REQTXRCNCL message
"""

import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

class ReqTxrcnclReceiver(P2PInterface):
    def __init__(self):
        super().__init__(support_txrcncl = True)
        self.reqtxrcncl_msg_received = None
        self.received_inv_items = 0

    def on_inv(self, message):
        self.received_inv_items += len(message.inv)
        super().on_inv(message)

    def on_reqtxrcncl(self, message):
        self.reqtxrcncl_msg_received = message

class ReqTxRcnclTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-txreconciliation']]

    def run_test(self):
        t = int(time.time())
        self.nodes[0].setmocktime(t)
        self.generate(self.nodes[0], COINBASE_MATURITY) # mature coinbase UTXO used later
        # Check everything about *sending* REQTXRCNCL.
        self.log.info('REQTXRCNCL sent to an outbound #1')
        peer = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=0)
        self.nodes[0].setmocktime(t + 8 + 1)
        self.wait_until(lambda: peer.reqtxrcncl_msg_received)
        assert_equal(peer.reqtxrcncl_msg_received.set_size, 0)
        assert_equal(peer.reqtxrcncl_msg_received.q, int(32767 * 0.25))
        self.nodes[0].disconnect_p2ps()

        self.log.info('REQTXRCNCL sent to an outbound #1 again, even though we added #2')
        with self.nodes[0].assert_debug_log(["Register peer=1", "Register peer=2"]):
            peer1 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=1)
            peer2 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=2)
        self.nodes[0].setmocktime(t + 16 + 1)
        self.wait_until(lambda: peer1.reqtxrcncl_msg_received)
        assert not peer2.reqtxrcncl_msg_received
        self.log.info('REQTXRCNCL sent to an outbound #2')
        peer1.reqtxrcncl_msg_received = None
        # Since there are two peers, it's half the delay.
        self.nodes[0].setmocktime(t + 20 + 1)
        self.wait_until(lambda: peer2.reqtxrcncl_msg_received)
        assert not peer1.reqtxrcncl_msg_received
        self.nodes[0].disconnect_p2ps()

        self.log.info('Check transactions announced (either low-fanout or reconciliation)')
        with self.nodes[0].assert_debug_log(["Register peer=3", "Register peer=4"]):
            peer1 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=3)
            peer2 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=4)

        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.rescan_utxos()
        utxos = self.wallet.get_utxos()
        for utxo in utxos:
            # We want all transactions to be childless.
            self.wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo)

        self.nodes[0].setmocktime(t + 24 + 1)
        self.wait_until(lambda: peer1.reqtxrcncl_msg_received)
        self.nodes[0].setmocktime(t + 28 + 1)
        self.wait_until(lambda: peer2.reqtxrcncl_msg_received)
        # Some of the announcements may go into the next reconciliation due to the random delay.
        # TODO: improve this check once it's possible to finalize this reconciliation
        #       and receive the next round.
        assert peer1.reqtxrcncl_msg_received.set_size + peer1.received_inv_items <= len(utxos)
        assert peer2.reqtxrcncl_msg_received.set_size + peer2.received_inv_items <= len(utxos)

if __name__ == '__main__':
    ReqTxRcnclTest().main()
