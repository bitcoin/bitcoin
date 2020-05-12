#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests NODE_COMPACT_FILTERS (BIP 157/158).

Tests that a node configured with -blockfilterindex and -peerblockfilters can serve
cfcheckpts.
"""

from test_framework.messages import (
    FILTER_TYPE_BASIC,
    msg_getcfcheckpt,
)
from test_framework.mininode import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    disconnect_nodes,
    wait_until,
)

class CompactFiltersTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.rpc_timeout = 480
        self.num_nodes = 2
        self.extra_args = [
            ["-blockfilterindex", "-peerblockfilters"],
            ["-blockfilterindex"],
        ]

    def run_test(self):
        # Node 0 supports COMPACT_FILTERS, node 1 does not.
        node0 = self.nodes[0].add_p2p_connection(P2PInterface())
        node1 = self.nodes[1].add_p2p_connection(P2PInterface())

        # Nodes 0 & 1 share the same first 999 blocks in the chain.
        self.nodes[0].generate(999)
        self.sync_blocks(timeout=600)

        # Stale blocks by disconnecting nodes 0 & 1, mining, then reconnecting
        disconnect_nodes(self.nodes[0], 1)

        self.nodes[0].generate(1)
        wait_until(lambda: self.nodes[0].getblockcount() == 1000)
        stale_block_hash = self.nodes[0].getblockhash(1000)

        self.nodes[1].generate(1001)
        wait_until(lambda: self.nodes[1].getblockcount() == 2000)

        self.log.info("get cfcheckpt on chain to be re-orged out.")
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(stale_block_hash, 16)
        )
        node0.send_and_ping(message=request)
        response = node0.last_message['cfcheckpt']
        assert_equal(response.filter_type, request.filter_type)
        assert_equal(response.stop_hash, request.stop_hash)
        assert_equal(len(response.headers), 1)

        self.log.info("Reorg node 0 to a new chain.")
        connect_nodes(self.nodes[0], 1)
        self.sync_blocks(timeout=600)

        main_block_hash = self.nodes[0].getblockhash(1000)
        assert main_block_hash != stale_block_hash, "node 0 chain did not reorganize"

        self.log.info("Check that peers can fetch cfcheckpt on active chain.")
        tip_hash = self.nodes[0].getbestblockhash()
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(tip_hash, 16)
        )
        node0.send_and_ping(request)
        response = node0.last_message['cfcheckpt']
        assert_equal(response.filter_type, request.filter_type)
        assert_equal(response.stop_hash, request.stop_hash)

        main_cfcheckpt = self.nodes[0].getblockfilter(main_block_hash, 'basic')['header']
        tip_cfcheckpt = self.nodes[0].getblockfilter(tip_hash, 'basic')['header']
        assert_equal(
            response.headers,
            [int(header, 16) for header in (main_cfcheckpt, tip_cfcheckpt)]
        )

        self.log.info("Check that peers can fetch cfcheckpt on stale chain.")
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(stale_block_hash, 16)
        )
        node0.send_and_ping(request)
        response = node0.last_message['cfcheckpt']

        stale_cfcheckpt = self.nodes[0].getblockfilter(stale_block_hash, 'basic')['header']
        assert_equal(
            response.headers,
            [int(header, 16) for header in (stale_cfcheckpt,)]
        )

        self.log.info("Requests to node 1 without NODE_COMPACT_FILTERS results in disconnection.")
        requests = [
            msg_getcfcheckpt(
                filter_type=FILTER_TYPE_BASIC,
                stop_hash=int(main_block_hash, 16)
            ),
        ]
        for request in requests:
            node1 = self.nodes[1].add_p2p_connection(P2PInterface())
            node1.send_message(request)
            node1.wait_for_disconnect()

        self.log.info("Check that invalid requests result in disconnection.")
        requests = [
            # Requesting unknown filter type results in disconnection.
            msg_getcfcheckpt(
                filter_type=255,
                stop_hash=int(main_block_hash, 16)
            ),
            # Requesting unknown hash results in disconnection.
            msg_getcfcheckpt(
                filter_type=FILTER_TYPE_BASIC,
                stop_hash=123456789,
            ),
        ]
        for request in requests:
            node0 = self.nodes[0].add_p2p_connection(P2PInterface())
            node0.send_message(request)
            node0.wait_for_disconnect()

if __name__ == '__main__':
    CompactFiltersTest().main()
