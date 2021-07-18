#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""This test covers tx-relay behaviors which are function of IBD
    * switching fee filter setting during and after IBD
    * stop tx-requesting during IBD
"""

from decimal import Decimal

from test_framework.messages import (
    COIN,
    CInv,
    MSG_WTX,
    msg_inv,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
    NONPREF_PEER_TX_DELAY,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

import time

MAX_FEE_FILTER = Decimal(9170997) / COIN
NORMAL_FEE_FILTER = Decimal(100) / COIN


class P2PIBDTxRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [
            ["-minrelaytxfee={}".format(NORMAL_FEE_FILTER)],
            ["-minrelaytxfee={}".format(NORMAL_FEE_FILTER)],
        ]

    def test_inv_ibd(self):
        self.log.info("Check that nodes don't request txn while still in IBD")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.send_message(msg_inv([CInv(t=MSG_WTX, h=0xffaa)]))
        # As tx-announcing connection is inbound, wait for the inbound delay applied by requester.

        mock_time = int(time.time() + NONPREF_PEER_TX_DELAY)
        self.nodes[0].setmocktime(mock_time)
        peer.sync_with_ping()
        with p2p_lock:
            assert_equal(peer.tx_getdata_count, 0)
        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def test_feefilter_ibd(self):
        self.log.info("Check that nodes set minfilter to MAX_MONEY while still in IBD")
        for node in self.nodes:
            assert node.getblockchaininfo()['initialblockdownload']
            self.wait_until(lambda: all(peer['minfeefilter'] == MAX_FEE_FILTER for peer in node.getpeerinfo()))

        # Come out of IBD by generating a block
        self.nodes[0].generate(1)
        self.sync_all()

        self.log.info("Check that nodes reset minfilter after coming out of IBD")
        for node in self.nodes:
            assert not node.getblockchaininfo()['initialblockdownload']
            self.wait_until(lambda: all(peer['minfeefilter'] == NORMAL_FEE_FILTER for peer in node.getpeerinfo()))

    def run_test(self):
        self.test_inv_ibd()
        self.test_feefilter_ibd()

if __name__ == '__main__':
    P2PIBDTxRelayTest().main()
