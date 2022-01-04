#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests disabletx message.

Test that bitcoind will disconnect us if we deviate from BIP338, and
send disallowed messages or invalid message combinations involving
disabletx."""

from test_framework.messages import (
    CInv,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    MSG_FILTERED_BLOCK,
    MSG_TX,
    MSG_WTX,
    MSG_WITNESS_FLAG,
    msg_disabletx,
    msg_feefilter,
    msg_filteradd,
    msg_filterclear,
    msg_filterload,
    msg_getdata,
    msg_inv,
    msg_mempool,
    msg_notfound,
    msg_tx,
    msg_version,
)
from test_framework.p2p import (
    P2P_SERVICES,
    P2P_SUBVERSION,
    P2P_VERSION,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework


class DisableTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-peerbloomfilters=1']]

    def expect_disabletx_for_outbound_block_relay_test(self):
        self.log.info('Check that bitcoind will send disabletx when making block-relay-only connections')
        peer = self.nodes[0].add_outbound_p2p_connection(P2PInterface(disabletx=True), p2p_idx=0, connection_type='block-relay-only')

        def test_function():
            return "disabletx" in peer.last_message

        peer.wait_until(test_function, timeout=60)
        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def disabletx_after_verack_test(self):
        self.log.info('Check for disconnection when sending disabletx after verack')
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.sync_with_ping()

        peer.send_message(msg_disabletx())
        peer.wait_for_disconnect()

    def disabletx_without_frelay_test(self):
        self.log.info('Check for disconnection when sending disabletx without fRelay=0')
        peer = self.nodes[0].add_p2p_connection(P2PInterface(disabletx=True), send_version=False, wait_for_verack=False)
        version = msg_version()
        version.nVersion = P2P_VERSION
        version.strSubVer = P2P_SUBVERSION
        version.relay = 1
        version.nServices = P2P_SERVICES
        version.addrTo.ip = peer.dstaddr
        version.addrTo.port = peer.dstport
        version.addrFrom.ip = "0.0.0.0"
        version.addrFrom.port = 0
        peer.send_message(version)

        peer.wait_for_disconnect()

    def disabletx_as_outbound_peer_test(self):
        self.log.info('Check for disconnection if we connect outbound to a disabletx peer')
        peer = self.nodes[0].add_outbound_p2p_connection(P2PInterface(disabletx=True), p2p_idx=0, connection_type='outbound-full-relay', expect_disconnect=True)
        peer.wait_for_disconnect()

    def getdata_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending getdata(tx) or getdata(merkleblock) after disabletx')
        for inv_type in (MSG_TX, MSG_TX | MSG_WITNESS_FLAG, MSG_WTX, MSG_FILTERED_BLOCK):
            wtxidrelay = False
            if inv_type == MSG_WTX:
                wtxidrelay = True
            peer = self.nodes[0].add_p2p_connection(P2PInterface(wtxidrelay=wtxidrelay, disabletx=True))
            peer.sync_with_ping()

            peer.send_message(msg_getdata(inv=[CInv(t=inv_type, h=self.tip_hash)]))
            peer.wait_for_disconnect()

    def inv_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending inv(tx) after disabletx')
        for inv_type in (MSG_TX, MSG_WTX):
            wtxidrelay = False
            if inv_type == MSG_WTX:
                wtxidrelay = True
            peer = self.nodes[0].add_p2p_connection(P2PInterface(wtxidrelay=wtxidrelay, disabletx=True))
            peer.sync_with_ping()

            peer.send_message(msg_inv(inv=[CInv(t=inv_type, h=0xdeadbeef)]))
            peer.wait_for_disconnect()

    def tx_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending a tx after disabletx')

        peer = self.nodes[0].add_p2p_connection(P2PInterface(disabletx=True))
        peer.sync_with_ping()

        # Construct a dummy transaction. Inputs will be missing so no
        # validation will occur (this is easier than constructing a valid
        # transaction, which should be unnecessary for this test).
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(0xdeadbeef, 0))]
        tx.vout = [CTxOut()]

        peer.send_message(msg_tx(tx))
        peer.wait_for_disconnect()

    def bloom_filter_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending a bip37 filter* message after disabletx')

        for msg in (msg_filterload(), msg_filterclear(), msg_filteradd(data=b'00')):
            peer = self.nodes[0].add_p2p_connection(P2PInterface(disabletx=True))
            peer.sync_with_ping()

            peer.send_message(msg)
            peer.wait_for_disconnect()

    def mempool_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending a mempool message after disabletx')

        peer = self.nodes[0].add_p2p_connection(P2PInterface(disabletx=True))
        peer.sync_with_ping()

        peer.send_message(msg_mempool())
        peer.wait_for_disconnect()

    def notfound_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending a notfound(tx) message after disabletx')

        for inv_type in (MSG_TX, MSG_WTX):
            wtxidrelay = False
            if inv_type == MSG_WTX:
                wtxidrelay = True
            peer = self.nodes[0].add_p2p_connection(P2PInterface(wtxidrelay=wtxidrelay, disabletx=True))
            peer.sync_with_ping()

            peer.send_message(msg_notfound(vec=[CInv(t=inv_type, h=0xdeadbeef)]))
            peer.wait_for_disconnect()

    def feefilter_after_disabletx_test(self):
        self.log.info('Check for disconnection when sending a feefilter message after disabletx')

        peer = self.nodes[0].add_p2p_connection(P2PInterface(disabletx=True))
        peer.sync_with_ping()

        peer.send_message(msg_feefilter())
        peer.wait_for_disconnect()

    def run_test(self):
        self.tip_hash = int(self.generate(self.nodes[0], 1)[0], 16)

        self.expect_disabletx_for_outbound_block_relay_test()
        self.disabletx_after_verack_test()
        self.disabletx_without_frelay_test()
        self.disabletx_as_outbound_peer_test()
        self.getdata_after_disabletx_test()
        self.inv_after_disabletx_test()
        self.tx_after_disabletx_test()
        self.bloom_filter_after_disabletx_test()
        self.mempool_after_disabletx_test()
        self.notfound_after_disabletx_test()
        self.feefilter_after_disabletx_test()


if __name__ == '__main__':
    DisableTxTest().main()
