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
from test_framework.mininode import (
    P2PInterface,
    mininode_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wait_until,
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
MAX_GETDATA_RANDOM_DELAY = 2  # seconds
INBOUND_PEER_TX_DELAY = 2  # seconds
TXID_RELAY_DELAY = 2 # seconds
MAX_GETDATA_IN_FLIGHT = 100
TX_EXPIRY_INTERVAL = GETDATA_TX_INTERVAL * 10

# Python test constants
NUM_INBOUND = 10
MAX_GETDATA_INBOUND_WAIT = GETDATA_TX_INTERVAL + MAX_GETDATA_RANDOM_DELAY + INBOUND_PEER_TX_DELAY + TXID_RELAY_DELAY


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
            with mininode_lock:
                return p.last_message.get("getdata") and p.last_message["getdata"].inv[-1].hash == txid

        node_0_mocktime = int(time.time())
        while outstanding_peer_index:
            node_0_mocktime += MAX_GETDATA_INBOUND_WAIT
            self.nodes[0].setmocktime(node_0_mocktime)
            wait_until(lambda: any(getdata_found(i) for i in outstanding_peer_index))
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
        timeout = 2 + (MAX_GETDATA_RANDOM_DELAY + INBOUND_PEER_TX_DELAY) + (
            GETDATA_TX_INTERVAL + MAX_GETDATA_RANDOM_DELAY)
        self.log.info("Tx should be received at node 1 after {} seconds".format(timeout))
        self.sync_mempools(timeout=timeout)

    def test_in_flight_max(self):
        self.log.info("Test that we don't request more than {} transactions from any peer, every {} minutes".format(
            MAX_GETDATA_IN_FLIGHT, TX_EXPIRY_INTERVAL / 60))
        txids = [i for i in range(MAX_GETDATA_IN_FLIGHT + 2)]

        p = self.nodes[0].p2ps[0]

        with mininode_lock:
            p.tx_getdata_count = 0

        p.send_message(msg_inv([CInv(t=MSG_WTX, h=i) for i in txids]))
        wait_until(lambda: p.tx_getdata_count >= MAX_GETDATA_IN_FLIGHT, lock=mininode_lock)
        with mininode_lock:
            assert_equal(p.tx_getdata_count, MAX_GETDATA_IN_FLIGHT)

        self.log.info("Now check that if we send a NOTFOUND for a transaction, we'll get one more request")
        p.send_message(msg_notfound(vec=[CInv(t=MSG_WTX, h=txids[0])]))
        wait_until(lambda: p.tx_getdata_count >= MAX_GETDATA_IN_FLIGHT + 1, timeout=10, lock=mininode_lock)
        with mininode_lock:
            assert_equal(p.tx_getdata_count, MAX_GETDATA_IN_FLIGHT + 1)

        WAIT_TIME = TX_EXPIRY_INTERVAL // 2 + TX_EXPIRY_INTERVAL
        self.log.info("if we wait about {} minutes, we should eventually get more requests".format(WAIT_TIME / 60))
        self.nodes[0].setmocktime(int(time.time() + WAIT_TIME))
        wait_until(lambda: p.tx_getdata_count == MAX_GETDATA_IN_FLIGHT + 2)
        self.nodes[0].setmocktime(0)

    def test_spurious_notfound(self):
        self.log.info('Check that spurious notfound is ignored')
        self.nodes[0].p2ps[0].send_message(msg_notfound(vec=[CInv(MSG_TX, 1)]))

    def run_test(self):
        # Setup the p2p connections
        self.peers = []
        for node in self.nodes:
            for _ in range(NUM_INBOUND):
                self.peers.append(node.add_p2p_connection(TestP2PConn()))

        self.log.info("Nodes are setup with {} incoming connections each".format(NUM_INBOUND))

        self.test_spurious_notfound()

        # Test the in-flight max first, because we want no transactions in
        # flight ahead of this test.
        self.test_in_flight_max()

        self.test_inv_block()

        self.test_tx_requests()


if __name__ == '__main__':
    TxDownloadTest().main()
