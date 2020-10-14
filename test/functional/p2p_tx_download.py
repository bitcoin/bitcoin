#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction download behavior
"""

from test_framework.messages import (
    CInv,
    CTransaction,
    FromHex,
    MSG_TX,
    MSG_TYPE_MASK,
    MSG_WTX,
    msg_inv,
    msg_notfound,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE

import time


class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.tx_getdata_count = 0

    def on_getdata(self, message):
        for i in message.inv:
            if i.type & MSG_TYPE_MASK == MSG_TX or i.type & MSG_TYPE_MASK == MSG_WTX:
                self.tx_getdata_count += 1


# Constants from net_processing
GETDATA_TX_INTERVAL = 60  # seconds
INBOUND_PEER_TX_DELAY = 2  # seconds
TXID_RELAY_DELAY = 2 # seconds
OVERLOADED_PEER_DELAY = 2 # seconds
MAX_GETDATA_IN_FLIGHT = 100
MAX_PEER_TX_ANNOUNCEMENTS = 5000

# Python test constants
NUM_INBOUND = 10
MAX_GETDATA_INBOUND_WAIT = GETDATA_TX_INTERVAL + INBOUND_PEER_TX_DELAY + TXID_RELAY_DELAY


class TxDownloadTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 2

    def test_tx_requests(self):
        self.log.info("Test that we request transactions from all our peers, eventually")

        txid = 0xdeadbeef

        self.log.info("Announce the txid from each incoming peer to node 0")
        msg = msg_inv([CInv(t=MSG_WTX, h=txid)])
        for p in self.nodes[0].p2ps:
            p.send_and_ping(msg)

        outstanding_peer_index = [i for i in range(len(self.nodes[0].p2ps))]

        def getdata_found(peer_index):
            p = self.nodes[0].p2ps[peer_index]
            with p2p_lock:
                return p.last_message.get("getdata") and p.last_message["getdata"].inv[-1].hash == txid

        node_0_mocktime = int(time.time())
        while outstanding_peer_index:
            node_0_mocktime += MAX_GETDATA_INBOUND_WAIT
            self.nodes[0].setmocktime(node_0_mocktime)
            self.wait_until(lambda: any(getdata_found(i) for i in outstanding_peer_index))
            for i in outstanding_peer_index:
                if getdata_found(i):
                    outstanding_peer_index.remove(i)

        self.nodes[0].setmocktime(0)
        self.log.info("All outstanding peers received a getdata")

    def test_inv_block(self):
        self.log.info("Generate a transaction on node 0")
        tx = self.nodes[0].createrawtransaction(
            inputs=[{  # coinbase
                "txid": self.nodes[0].getblock(self.nodes[0].getblockhash(1))['tx'][0],
                "vout": 0
            }],
            outputs={ADDRESS_BCRT1_UNSPENDABLE: 50 - 0.00025},
        )
        tx = self.nodes[0].signrawtransactionwithkey(
            hexstring=tx,
            privkeys=[self.nodes[0].get_deterministic_priv_key().key],
        )['hex']
        ctx = FromHex(CTransaction(), tx)
        txid = int(ctx.rehash(), 16)

        self.log.info(
            "Announce the transaction to all nodes from all {} incoming peers, but never send it".format(NUM_INBOUND))
        msg = msg_inv([CInv(t=MSG_TX, h=txid)])
        for p in self.peers:
            p.send_and_ping(msg)

        self.log.info("Put the tx in node 0's mempool")
        self.nodes[0].sendrawtransaction(tx)

        # Since node 1 is connected outbound to an honest peer (node 0), it
        # should get the tx within a timeout. (Assuming that node 0
        # announced the tx within the timeout)
        # The timeout is the sum of
        # * the worst case until the tx is first requested from an inbound
        #   peer, plus
        # * the first time it is re-requested from the outbound peer, plus
        # * 2 seconds to avoid races
        assert self.nodes[1].getpeerinfo()[0]['inbound'] == False
        timeout = 2 + INBOUND_PEER_TX_DELAY + GETDATA_TX_INTERVAL
        self.log.info("Tx should be received at node 1 after {} seconds".format(timeout))
        self.sync_mempools(timeout=timeout)

    def test_in_flight_max(self):
        self.log.info("Test that we don't load peers with more than {} transaction requests immediately".format(MAX_GETDATA_IN_FLIGHT))
        txids = [i for i in range(MAX_GETDATA_IN_FLIGHT + 2)]

        p = self.nodes[0].p2ps[0]

        with p2p_lock:
            p.tx_getdata_count = 0

        mock_time = int(time.time() + 1)
        self.nodes[0].setmocktime(mock_time)
        for i in range(MAX_GETDATA_IN_FLIGHT):
            p.send_message(msg_inv([CInv(t=MSG_WTX, h=txids[i])]))
        p.sync_with_ping()
        mock_time += INBOUND_PEER_TX_DELAY
        self.nodes[0].setmocktime(mock_time)
        p.wait_until(lambda: p.tx_getdata_count >= MAX_GETDATA_IN_FLIGHT)
        for i in range(MAX_GETDATA_IN_FLIGHT, len(txids)):
            p.send_message(msg_inv([CInv(t=MSG_WTX, h=txids[i])]))
        p.sync_with_ping()
        self.log.info("No more than {} requests should be seen within {} seconds after announcement".format(MAX_GETDATA_IN_FLIGHT, INBOUND_PEER_TX_DELAY + OVERLOADED_PEER_DELAY - 1))
        self.nodes[0].setmocktime(mock_time + INBOUND_PEER_TX_DELAY + OVERLOADED_PEER_DELAY - 1)
        p.sync_with_ping()
        with p2p_lock:
            assert_equal(p.tx_getdata_count, MAX_GETDATA_IN_FLIGHT)
        self.log.info("If we wait {} seconds after announcement, we should eventually get more requests".format(INBOUND_PEER_TX_DELAY + OVERLOADED_PEER_DELAY))
        self.nodes[0].setmocktime(mock_time + INBOUND_PEER_TX_DELAY + OVERLOADED_PEER_DELAY)
        p.wait_until(lambda: p.tx_getdata_count == len(txids))

    def test_expiry_fallback(self):
        self.log.info('Check that expiry will select another peer for download')
        WTXID = 0xffaa
        peer1 = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer2 = self.nodes[0].add_p2p_connection(TestP2PConn())
        for p in [peer1, peer2]:
            p.send_message(msg_inv([CInv(t=MSG_WTX, h=WTXID)]))
        # One of the peers is asked for the tx
        peer2.wait_until(lambda: sum(p.tx_getdata_count for p in [peer1, peer2]) == 1)
        with p2p_lock:
            peer_expiry, peer_fallback = (peer1, peer2) if peer1.tx_getdata_count == 1 else (peer2, peer1)
            assert_equal(peer_fallback.tx_getdata_count, 0)
        self.nodes[0].setmocktime(int(time.time()) + GETDATA_TX_INTERVAL + 1)  # Wait for request to peer_expiry to expire
        peer_fallback.wait_until(lambda: peer_fallback.tx_getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer_fallback.tx_getdata_count, 1)
        self.restart_node(0)  # reset mocktime

    def test_disconnect_fallback(self):
        self.log.info('Check that disconnect will select another peer for download')
        WTXID = 0xffbb
        peer1 = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer2 = self.nodes[0].add_p2p_connection(TestP2PConn())
        for p in [peer1, peer2]:
            p.send_message(msg_inv([CInv(t=MSG_WTX, h=WTXID)]))
        # One of the peers is asked for the tx
        peer2.wait_until(lambda: sum(p.tx_getdata_count for p in [peer1, peer2]) == 1)
        with p2p_lock:
            peer_disconnect, peer_fallback = (peer1, peer2) if peer1.tx_getdata_count == 1 else (peer2, peer1)
            assert_equal(peer_fallback.tx_getdata_count, 0)
        peer_disconnect.peer_disconnect()
        peer_disconnect.wait_for_disconnect()
        peer_fallback.wait_until(lambda: peer_fallback.tx_getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer_fallback.tx_getdata_count, 1)

    def test_notfound_fallback(self):
        self.log.info('Check that notfounds will select another peer for download immediately')
        WTXID = 0xffdd
        peer1 = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer2 = self.nodes[0].add_p2p_connection(TestP2PConn())
        for p in [peer1, peer2]:
            p.send_message(msg_inv([CInv(t=MSG_WTX, h=WTXID)]))
        # One of the peers is asked for the tx
        peer2.wait_until(lambda: sum(p.tx_getdata_count for p in [peer1, peer2]) == 1)
        with p2p_lock:
            peer_notfound, peer_fallback = (peer1, peer2) if peer1.tx_getdata_count == 1 else (peer2, peer1)
            assert_equal(peer_fallback.tx_getdata_count, 0)
        peer_notfound.send_and_ping(msg_notfound(vec=[CInv(MSG_WTX, WTXID)]))  # Send notfound, so that fallback peer is selected
        peer_fallback.wait_until(lambda: peer_fallback.tx_getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer_fallback.tx_getdata_count, 1)

    def test_preferred_inv(self):
        self.log.info('Check that invs from preferred peers are downloaded immediately')
        self.restart_node(0, extra_args=['-whitelist=noban@127.0.0.1'])
        peer = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer.send_message(msg_inv([CInv(t=MSG_WTX, h=0xff00ff00)]))
        peer.wait_until(lambda: peer.tx_getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer.tx_getdata_count, 1)

    def test_large_inv_batch(self):
        self.log.info('Test how large inv batches are handled with relay permission')
        self.restart_node(0, extra_args=['-whitelist=relay@127.0.0.1'])
        peer = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer.send_message(msg_inv([CInv(t=MSG_WTX, h=wtxid) for wtxid in range(MAX_PEER_TX_ANNOUNCEMENTS + 1)]))
        peer.wait_until(lambda: peer.tx_getdata_count == MAX_PEER_TX_ANNOUNCEMENTS + 1)

        self.log.info('Test how large inv batches are handled without relay permission')
        self.restart_node(0)
        peer = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer.send_message(msg_inv([CInv(t=MSG_WTX, h=wtxid) for wtxid in range(MAX_PEER_TX_ANNOUNCEMENTS + 1)]))
        peer.wait_until(lambda: peer.tx_getdata_count == MAX_PEER_TX_ANNOUNCEMENTS)
        peer.sync_with_ping()
        with p2p_lock:
            assert_equal(peer.tx_getdata_count, MAX_PEER_TX_ANNOUNCEMENTS)

    def test_spurious_notfound(self):
        self.log.info('Check that spurious notfound is ignored')
        self.nodes[0].p2ps[0].send_message(msg_notfound(vec=[CInv(MSG_TX, 1)]))

    def run_test(self):
        # Run tests without mocktime that only need one peer-connection first, to avoid restarting the nodes
        self.test_expiry_fallback()
        self.test_disconnect_fallback()
        self.test_notfound_fallback()
        self.test_preferred_inv()
        self.test_large_inv_batch()
        self.test_spurious_notfound()

        # Run each test against new bitcoind instances, as setting mocktimes has long-term effects on when
        # the next trickle relay event happens.
        for test in [self.test_in_flight_max, self.test_inv_block, self.test_tx_requests]:
            self.stop_nodes()
            self.start_nodes()
            self.connect_nodes(1, 0)
            # Setup the p2p connections
            self.peers = []
            for node in self.nodes:
                for _ in range(NUM_INBOUND):
                    self.peers.append(node.add_p2p_connection(TestP2PConn()))
            self.log.info("Nodes are setup with {} incoming connections each".format(NUM_INBOUND))
            test()


if __name__ == '__main__':
    TxDownloadTest().main()
