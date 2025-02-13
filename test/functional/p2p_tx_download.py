#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction download behavior
"""
from decimal import Decimal
from enum import Enum
import time

from test_framework.mempool_util import (
    fill_mempool,
)
from test_framework.messages import (
    CInv,
    MSG_TX,
    MSG_TYPE_MASK,
    MSG_WTX,
    msg_inv,
    msg_notfound,
    msg_tx,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
    NONPREF_PEER_TX_DELAY,
    GETDATA_TX_INTERVAL,
    TXID_RELAY_DELAY,
    OVERLOADED_PEER_TX_DELAY
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet


class TestP2PConn(P2PInterface):
    def __init__(self, wtxidrelay=True):
        super().__init__(wtxidrelay=wtxidrelay)
        self.tx_getdata_count = 0

    def on_getdata(self, message):
        for i in message.inv:
            if i.type & MSG_TYPE_MASK == MSG_TX or i.type & MSG_TYPE_MASK == MSG_WTX:
                self.tx_getdata_count += 1


# Constants from txdownloadman
MAX_PEER_TX_REQUEST_IN_FLIGHT = 100
MAX_PEER_TX_ANNOUNCEMENTS = 5000

# Python test constants
NUM_INBOUND = 10
MAX_GETDATA_INBOUND_WAIT = GETDATA_TX_INTERVAL + NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY

class ConnectionType(Enum):
    """ Different connection types
    1. INBOUND: Incoming connection, not whitelisted
    2. OUTBOUND: Outgoing connection
    3. WHITELIST: Incoming connection, but whitelisted
    """
    INBOUND = 0
    OUTBOUND = 1
    WHITELIST = 2

class TxDownloadTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args= [['-datacarriersize=100000', '-maxmempool=5', '-persistmempool=0']] * self.num_nodes

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
        tx = self.wallet.create_self_transfer()
        wtxid = int(tx['wtxid'], 16)

        self.nodes[0].setmocktime(int(time.time()))

        self.log.info(
            "Announce the transaction to all nodes from all {} incoming peers, but never send it".format(NUM_INBOUND))
        msg = msg_inv([CInv(t=MSG_WTX, h=wtxid)])
        for p in self.peers:
            p.send_and_ping(msg)

        self.log.info("Put the tx in node 0's mempool")
        self.nodes[0].sendrawtransaction(tx['hex'])

        # Since node 1 is connected outbound to an honest peer (node 0), it
        # should get the tx within a timeout. (Assuming that node 0
        # announced the tx within the timeout)
        # The timeout is the sum of
        # * the worst case until the tx is first requested from an inbound
        #   peer, plus
        # * the first time it is re-requested from the outbound peer, plus
        # * 2 seconds to avoid races
        assert self.nodes[1].getpeerinfo()[0]['inbound'] == False
        timeout = 2 + NONPREF_PEER_TX_DELAY + GETDATA_TX_INTERVAL
        self.log.info("Tx should be received at node 1 after {} seconds".format(timeout))
        self.nodes[0].bumpmocktime(timeout)
        self.sync_mempools()

        self.nodes[0].setmocktime(0)

    def test_in_flight_max(self):
        self.log.info("Test that we don't load peers with more than {} transaction requests immediately".format(MAX_PEER_TX_REQUEST_IN_FLIGHT))
        txids = [i for i in range(MAX_PEER_TX_REQUEST_IN_FLIGHT + 2)]

        p = self.nodes[0].p2ps[0]

        with p2p_lock:
            p.tx_getdata_count = 0

        mock_time = int(time.time() + 1)
        self.nodes[0].setmocktime(mock_time)
        for i in range(MAX_PEER_TX_REQUEST_IN_FLIGHT):
            p.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=txids[i])]))
        p.sync_with_ping()
        mock_time += NONPREF_PEER_TX_DELAY
        self.nodes[0].setmocktime(mock_time)
        p.wait_until(lambda: p.tx_getdata_count >= MAX_PEER_TX_REQUEST_IN_FLIGHT)
        for i in range(MAX_PEER_TX_REQUEST_IN_FLIGHT, len(txids)):
            p.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=txids[i])]))
        p.sync_with_ping()
        self.log.info("No more than {} requests should be seen within {} seconds after announcement".format(MAX_PEER_TX_REQUEST_IN_FLIGHT, NONPREF_PEER_TX_DELAY + OVERLOADED_PEER_TX_DELAY - 1))
        self.nodes[0].setmocktime(mock_time + NONPREF_PEER_TX_DELAY + OVERLOADED_PEER_TX_DELAY - 1)
        p.sync_with_ping()
        with p2p_lock:
            assert_equal(p.tx_getdata_count, MAX_PEER_TX_REQUEST_IN_FLIGHT)
        self.log.info("If we wait {} seconds after announcement, we should eventually get more requests".format(NONPREF_PEER_TX_DELAY + OVERLOADED_PEER_TX_DELAY))
        self.nodes[0].setmocktime(mock_time + NONPREF_PEER_TX_DELAY + OVERLOADED_PEER_TX_DELAY)
        p.wait_until(lambda: p.tx_getdata_count == len(txids))

    def test_expiry_fallback(self):
        self.log.info('Check that expiry will select another peer for download')
        WTXID = 0xffaa
        peer1 = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer2 = self.nodes[0].add_p2p_connection(TestP2PConn())
        for p in [peer1, peer2]:
            p.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=WTXID)]))
        # One of the peers is asked for the tx
        peer2.wait_until(lambda: sum(p.tx_getdata_count for p in [peer1, peer2]) == 1)
        with p2p_lock:
            _peer_expiry, peer_fallback = (peer1, peer2) if peer1.tx_getdata_count == 1 else (peer2, peer1)
            assert_equal(peer_fallback.tx_getdata_count, 0)
        self.nodes[0].setmocktime(int(time.time()) + GETDATA_TX_INTERVAL + 1)  # Wait for request to _peer_expiry to expire
        peer_fallback.wait_until(lambda: peer_fallback.tx_getdata_count >= 1, timeout=1)
        self.restart_node(0)  # reset mocktime

    def test_disconnect_fallback(self):
        self.log.info('Check that disconnect will select another peer for download')
        WTXID = 0xffbb
        peer1 = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer2 = self.nodes[0].add_p2p_connection(TestP2PConn())
        for p in [peer1, peer2]:
            p.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=WTXID)]))
        # One of the peers is asked for the tx
        peer2.wait_until(lambda: sum(p.tx_getdata_count for p in [peer1, peer2]) == 1)
        with p2p_lock:
            peer_disconnect, peer_fallback = (peer1, peer2) if peer1.tx_getdata_count == 1 else (peer2, peer1)
            assert_equal(peer_fallback.tx_getdata_count, 0)
        peer_disconnect.peer_disconnect()
        peer_disconnect.wait_for_disconnect()
        peer_fallback.wait_until(lambda: peer_fallback.tx_getdata_count >= 1, timeout=1)

    def test_notfound_fallback(self):
        self.log.info('Check that notfounds will select another peer for download immediately')
        WTXID = 0xffdd
        peer1 = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer2 = self.nodes[0].add_p2p_connection(TestP2PConn())
        for p in [peer1, peer2]:
            p.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=WTXID)]))
        # One of the peers is asked for the tx
        peer2.wait_until(lambda: sum(p.tx_getdata_count for p in [peer1, peer2]) == 1)
        with p2p_lock:
            peer_notfound, peer_fallback = (peer1, peer2) if peer1.tx_getdata_count == 1 else (peer2, peer1)
            assert_equal(peer_fallback.tx_getdata_count, 0)
        peer_notfound.send_and_ping(msg_notfound(vec=[CInv(MSG_WTX, WTXID)]))  # Send notfound, so that fallback peer is selected
        peer_fallback.wait_until(lambda: peer_fallback.tx_getdata_count >= 1, timeout=1)

    def test_preferred_inv(self, connection_type: ConnectionType):
        if connection_type == ConnectionType.WHITELIST:
            self.log.info('Check invs from preferred (whitelisted) peers are downloaded immediately')
            self.restart_node(0, extra_args=['-whitelist=noban@127.0.0.1'])
        elif connection_type == ConnectionType.OUTBOUND:
            self.log.info('Check invs from preferred (outbound) peers are downloaded immediately')
            self.restart_node(0)
        elif connection_type == ConnectionType.INBOUND:
            self.log.info('Check invs from non-preferred peers are downloaded after {} s'.format(NONPREF_PEER_TX_DELAY))
            self.restart_node(0)
        else:
            raise Exception("invalid connection_type")

        mock_time = int(time.time() + 1)
        self.nodes[0].setmocktime(mock_time)

        if connection_type == ConnectionType.OUTBOUND:
            peer = self.nodes[0].add_outbound_p2p_connection(
               TestP2PConn(), wait_for_verack=True, p2p_idx=1, connection_type="outbound-full-relay")
        else:
            peer = self.nodes[0].add_p2p_connection(TestP2PConn())

        peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=0xff00ff00)]))
        if connection_type != ConnectionType.INBOUND:
            peer.wait_until(lambda: peer.tx_getdata_count >= 1, timeout=1)
        else:
            with p2p_lock:
                assert_equal(peer.tx_getdata_count, 0)
            self.nodes[0].setmocktime(mock_time + NONPREF_PEER_TX_DELAY)
            peer.wait_until(lambda: peer.tx_getdata_count >= 1, timeout=1)

    def test_preferred_tiebreaker_inv(self):
        self.log.info("Test that preferred peers are always selected over non-preferred when ready")

        self.restart_node(0)
        self.nodes[0].setmocktime(int(time.time()))

        # Peer that is immediately asked, but never responds.
        # This will set us up to have two ready requests, one
        # of which is preferred and one which is not
        unresponsive_peer = self.nodes[0].add_outbound_p2p_connection(
           TestP2PConn(), wait_for_verack=True, p2p_idx=0, connection_type="outbound-full-relay")
        unresponsive_peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=0xff00ff00)]))
        unresponsive_peer.wait_until(lambda: unresponsive_peer.tx_getdata_count >= 1, timeout=1)

        # A bunch of incoming (non-preferred) connections that advertise the same tx
        non_pref_peers = []
        NUM_INBOUND = 10
        for _ in range(NUM_INBOUND):
            non_pref_peers.append(self.nodes[0].add_p2p_connection(TestP2PConn()))
            non_pref_peers[-1].send_and_ping(msg_inv([CInv(t=MSG_WTX, h=0xff00ff00)]))

        # Check that no request made due to in-flight
        self.nodes[0].bumpmocktime(NONPREF_PEER_TX_DELAY)
        with p2p_lock:
            for peer in non_pref_peers:
                    assert_equal(peer.tx_getdata_count, 0)

        # Now add another outbound (preferred) which is immediately ready for consideration
        # upon advertisement
        pref_peer = self.nodes[0].add_outbound_p2p_connection(
           TestP2PConn(), wait_for_verack=True, p2p_idx=1, connection_type="outbound-full-relay")
        pref_peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=0xff00ff00)]))

        assert_equal(len(self.nodes[0].getpeerinfo()), NUM_INBOUND + 2)

        # Still have to wait for in-flight to timeout
        with p2p_lock:
            assert_equal(pref_peer.tx_getdata_count, 0)

        # Timeout in-flight
        self.nodes[0].bumpmocktime(GETDATA_TX_INTERVAL - NONPREF_PEER_TX_DELAY)

        # Preferred peers are *always* selected next if ready
        pref_peer.wait_until(lambda: pref_peer.tx_getdata_count >= 1, timeout=10)

        # And none for non-preferred
        for non_pref_peer in non_pref_peers:
            with p2p_lock:
                assert_equal(non_pref_peer.tx_getdata_count, 0)

    def test_txid_inv_delay(self, glob_wtxid=False):
        self.log.info('Check that inv from a txid-relay peers are delayed by {} s, with a wtxid peer {}'.format(TXID_RELAY_DELAY, glob_wtxid))
        self.restart_node(0, extra_args=['-whitelist=noban@127.0.0.1'])
        mock_time = int(time.time() + 1)
        self.nodes[0].setmocktime(mock_time)
        peer = self.nodes[0].add_p2p_connection(TestP2PConn(wtxidrelay=False))
        if glob_wtxid:
            # Add a second wtxid-relay connection otherwise TXID_RELAY_DELAY is waived in
            # lack of wtxid-relay peers
            self.nodes[0].add_p2p_connection(TestP2PConn(wtxidrelay=True))
        peer.send_and_ping(msg_inv([CInv(t=MSG_TX, h=0xff11ff11)]))
        with p2p_lock:
            assert_equal(peer.tx_getdata_count, 0 if glob_wtxid else 1)
        self.nodes[0].setmocktime(mock_time + TXID_RELAY_DELAY)
        peer.wait_until(lambda: peer.tx_getdata_count >= 1, timeout=1)

    def test_large_inv_batch(self):
        self.log.info('Test how large inv batches are handled with relay permission')
        self.restart_node(0, extra_args=['-whitelist=relay@127.0.0.1'])
        peer = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=wtxid) for wtxid in range(MAX_PEER_TX_ANNOUNCEMENTS + 1)]))
        peer.wait_until(lambda: peer.tx_getdata_count == MAX_PEER_TX_ANNOUNCEMENTS + 1)

        self.log.info('Test how large inv batches are handled without relay permission')
        self.restart_node(0)
        peer = self.nodes[0].add_p2p_connection(TestP2PConn())
        peer.send_without_ping(msg_inv([CInv(t=MSG_WTX, h=wtxid) for wtxid in range(MAX_PEER_TX_ANNOUNCEMENTS + 1)]))
        peer.wait_until(lambda: peer.tx_getdata_count == MAX_PEER_TX_ANNOUNCEMENTS)
        peer.sync_with_ping()

    def test_spurious_notfound(self):
        self.log.info('Check that spurious notfound is ignored')
        self.nodes[0].p2ps[0].send_without_ping(msg_notfound(vec=[CInv(MSG_TX, 1)]))

    def test_rejects_filter_reset(self):
        self.log.info('Check that rejected tx is not requested again')
        node = self.nodes[0]
        fill_mempool(self, node, tx_sync_fun=self.no_op)
        self.wallet.rescan_utxos()
        mempoolminfee = node.getmempoolinfo()['mempoolminfee']
        peer = node.add_p2p_connection(TestP2PConn())
        low_fee_tx = self.wallet.create_self_transfer(fee_rate=Decimal("0.9")*mempoolminfee)
        assert_equal(node.testmempoolaccept([low_fee_tx['hex']])[0]["reject-reason"], "mempool min fee not met")
        peer.send_and_ping(msg_tx(low_fee_tx['tx']))
        peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=int(low_fee_tx['wtxid'], 16))]))
        node.setmocktime(int(time.time()))
        node.bumpmocktime(MAX_GETDATA_INBOUND_WAIT)
        peer.sync_with_ping()
        assert_equal(peer.tx_getdata_count, 0)

        self.log.info('Check that rejection filter is cleared after new block comes in')
        self.generate(self.wallet, 1, sync_fun=self.no_op)
        peer.sync_with_ping()
        peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=int(low_fee_tx['wtxid'], 16))]))
        node.bumpmocktime(MAX_GETDATA_INBOUND_WAIT)
        peer.wait_for_getdata([int(low_fee_tx['wtxid'], 16)])

    def test_inv_wtxidrelay_mismatch(self):
        self.log.info("Check that INV messages that don't match the wtxidrelay setting are ignored")
        node = self.nodes[0]
        wtxidrelay_on_peer = node.add_p2p_connection(TestP2PConn(wtxidrelay=True))
        wtxidrelay_off_peer = node.add_p2p_connection(TestP2PConn(wtxidrelay=False))
        random_tx = self.wallet.create_self_transfer()

        # MSG_TX INV from wtxidrelay=True peer -> mismatch, ignored
        wtxidrelay_on_peer.send_and_ping(msg_inv([CInv(t=MSG_TX, h=int(random_tx['txid'], 16))]))
        node.setmocktime(int(time.time()))
        node.bumpmocktime(MAX_GETDATA_INBOUND_WAIT)
        wtxidrelay_on_peer.sync_with_ping()
        assert_equal(wtxidrelay_on_peer.tx_getdata_count, 0)

        # MSG_WTX INV from wtxidrelay=False peer -> mismatch, ignored
        wtxidrelay_off_peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=int(random_tx['wtxid'], 16))]))
        node.bumpmocktime(MAX_GETDATA_INBOUND_WAIT)
        wtxidrelay_off_peer.sync_with_ping()
        assert_equal(wtxidrelay_off_peer.tx_getdata_count, 0)

        # MSG_TX INV from wtxidrelay=False peer works
        wtxidrelay_off_peer.send_and_ping(msg_inv([CInv(t=MSG_TX, h=int(random_tx['txid'], 16))]))
        node.bumpmocktime(MAX_GETDATA_INBOUND_WAIT)
        wtxidrelay_off_peer.wait_for_getdata([int(random_tx['txid'], 16)])

        # MSG_WTX INV from wtxidrelay=True peer works
        wtxidrelay_on_peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=int(random_tx['wtxid'], 16))]))
        node.bumpmocktime(MAX_GETDATA_INBOUND_WAIT)
        wtxidrelay_on_peer.wait_for_getdata([int(random_tx['wtxid'], 16)])

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Run tests without mocktime that only need one peer-connection first, to avoid restarting the nodes
        self.test_expiry_fallback()
        self.test_disconnect_fallback()
        self.test_notfound_fallback()
        self.test_preferred_tiebreaker_inv()
        self.test_preferred_inv(ConnectionType.INBOUND)
        self.test_preferred_inv(ConnectionType.OUTBOUND)
        self.test_preferred_inv(ConnectionType.WHITELIST)
        self.test_txid_inv_delay()
        self.test_txid_inv_delay(True)
        self.test_large_inv_batch()
        self.test_spurious_notfound()

        # Run each test against new bitcoind instances, as setting mocktimes has long-term effects on when
        # the next trickle relay event happens.
        for test, with_inbounds in [
            (self.test_in_flight_max, True),
            (self.test_inv_block, True),
            (self.test_tx_requests, True),
            (self.test_rejects_filter_reset, False),
            (self.test_inv_wtxidrelay_mismatch, False),
        ]:
            self.stop_nodes()
            self.start_nodes()
            self.connect_nodes(1, 0)
            # Setup the p2p connections
            self.peers = []
            if with_inbounds:
                for node in self.nodes:
                    for _ in range(NUM_INBOUND):
                        self.peers.append(node.add_p2p_connection(TestP2PConn()))
                self.log.info("Nodes are setup with {} incoming connections each".format(NUM_INBOUND))
            test()

if __name__ == '__main__':
    TxDownloadTest(__file__).main()
