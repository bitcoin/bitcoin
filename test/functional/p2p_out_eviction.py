#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    msg_pong,
    msg_tx,
    msg_feefilter,
)
from test_framework.p2p import (
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, check_node_connections
from test_framework.wallet import MiniWallet

class P2POutEvict(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Start with all the same fee filters.
        self.extra_args = [["-minrelaytxfee=0.00000100"]]

    def mock_forward(self, delta):
        self.mock_time += delta
        self.nodes[0].setmocktime(self.mock_time)

    def run_test(self):
        self.mock_time = int(time.time())
        self.mock_forward(0)
        node = self.nodes[0]
        peers = []
        for id in range(8):
            peers.append(node.add_outbound_p2p_connection(P2PDataStore(), p2p_idx=id, connection_type="outbound-full-relay"))

        node.mockscheduler(60)
        check_node_connections(node=node, num_in=0, num_out=8)

        # Set a limited number of fee filters. Nothing is evicted because
        # the rest of the peers are sufficient.
        for id in range(4):
            peers[id].send_and_ping(msg_feefilter(10))
        node.mockscheduler(60)
        check_node_connections(node=node, num_in=0, num_out=8)

        # Set one more filter, but make it borderline acceptable.
        peers[4].send_and_ping(msg_feefilter(81))
        node.mockscheduler(60)
        check_node_connections(node=node, num_in=0, num_out=8)

        # Now there is too few peers with a comparable min tx relay fee,
        # thus one of the peers must be evicted to have a room for a new peer
        # (hopefully with sufficient fee filter).
        peers[4].send_and_ping(msg_feefilter(10))
        node.mockscheduler(60)
        self.mock_forward(60)
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 7, timeout=10)

if __name__ == '__main__':
    P2POutEvict().main()
