#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests NODE_COMPACT_FILTERS (BIP 157/158).

Tests that a node configured with -blockfilterindex and -peerblockfilters signals
NODE_COMPACT_FILTERS and can serve cfilters, cfheaders and cfcheckpts.
"""

from test_framework.messages import (
    FILTER_TYPE_BASIC,
    NODE_COMPACT_FILTERS,
    hash256,
    msg_getcfcheckpt,
    msg_getcfheaders,
    msg_getcfilters,
    ser_uint256,
    uint256_from_str,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

class FiltersClient(P2PInterface):
    def __init__(self):
        super().__init__()
        # Store the cfilters received.
        self.cfilters = []

    def pop_cfilters(self):
        cfilters = self.cfilters
        self.cfilters = []
        return cfilters

    def on_cfilter(self, message):
        """Store cfilters received in a list."""
        self.cfilters.append(message)


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
        peer_0 = self.nodes[0].add_p2p_connection(FiltersClient())
        peer_1 = self.nodes[1].add_p2p_connection(FiltersClient())

        # Nodes 0 & 1 share the same first 999 blocks in the chain.
        self.nodes[0].generate(999)
        self.sync_blocks(timeout=600)

        # Stale blocks by disconnecting nodes 0 & 1, mining, then reconnecting
        self.disconnect_nodes(0, 1)

        stale_block_hash = self.nodes[0].generate(1)[0]
        assert_equal(self.nodes[0].getblockcount(), 1000)

        self.nodes[1].generate(1001)
        assert_equal(self.nodes[1].getblockcount(), 2000)

        # Check that nodes have signalled NODE_COMPACT_FILTERS correctly.
        assert peer_0.nServices & NODE_COMPACT_FILTERS != 0
        assert peer_1.nServices & NODE_COMPACT_FILTERS == 0

        # Check that the localservices is as expected.
        assert int(self.nodes[0].getnetworkinfo()['localservices'], 16) & NODE_COMPACT_FILTERS != 0
        assert int(self.nodes[1].getnetworkinfo()['localservices'], 16) & NODE_COMPACT_FILTERS == 0

        self.log.info("get cfcheckpt on chain to be re-orged out.")
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(message=request)
        response = peer_0.last_message['cfcheckpt']
        assert_equal(response.filter_type, request.filter_type)
        assert_equal(response.stop_hash, request.stop_hash)
        assert_equal(len(response.headers), 1)

        self.log.info("Reorg node 0 to a new chain.")
        self.connect_nodes(0, 1)
        self.sync_blocks(timeout=600)

        main_block_hash = self.nodes[0].getblockhash(1000)
        assert main_block_hash != stale_block_hash, "node 0 chain did not reorganize"

        self.log.info("Check that peers can fetch cfcheckpt on active chain.")
        tip_hash = self.nodes[0].getbestblockhash()
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(tip_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfcheckpt']
        assert_equal(response.filter_type, request.filter_type)
        assert_equal(response.stop_hash, request.stop_hash)

        main_cfcheckpt = self.nodes[0].getblockfilter(main_block_hash, 'basic')['header']
        tip_cfcheckpt = self.nodes[0].getblockfilter(tip_hash, 'basic')['header']
        assert_equal(
            response.headers,
            [int(header, 16) for header in (main_cfcheckpt, tip_cfcheckpt)],
        )

        self.log.info("Check that peers can fetch cfcheckpt on stale chain.")
        request = msg_getcfcheckpt(
            filter_type=FILTER_TYPE_BASIC,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfcheckpt']

        stale_cfcheckpt = self.nodes[0].getblockfilter(stale_block_hash, 'basic')['header']
        assert_equal(
            response.headers,
            [int(header, 16) for header in (stale_cfcheckpt, )],
        )

        self.log.info("Check that peers can fetch cfheaders on active chain.")
        request = msg_getcfheaders(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1,
            stop_hash=int(main_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfheaders']
        main_cfhashes = response.hashes
        assert_equal(len(main_cfhashes), 1000)
        assert_equal(
            compute_last_header(response.prev_header, response.hashes),
            int(main_cfcheckpt, 16),
        )

        self.log.info("Check that peers can fetch cfheaders on stale chain.")
        request = msg_getcfheaders(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.last_message['cfheaders']
        stale_cfhashes = response.hashes
        assert_equal(len(stale_cfhashes), 1000)
        assert_equal(
            compute_last_header(response.prev_header, response.hashes),
            int(stale_cfcheckpt, 16),
        )

        self.log.info("Check that peers can fetch cfilters.")
        stop_hash = self.nodes[0].getblockhash(10)
        request = msg_getcfilters(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1,
            stop_hash=int(stop_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.pop_cfilters()
        assert_equal(len(response), 10)

        self.log.info("Check that cfilter responses are correct.")
        for cfilter, cfhash, height in zip(response, main_cfhashes, range(1, 11)):
            block_hash = self.nodes[0].getblockhash(height)
            assert_equal(cfilter.filter_type, FILTER_TYPE_BASIC)
            assert_equal(cfilter.block_hash, int(block_hash, 16))
            computed_cfhash = uint256_from_str(hash256(cfilter.filter_data))
            assert_equal(computed_cfhash, cfhash)

        self.log.info("Check that peers can fetch cfilters for stale blocks.")
        request = msg_getcfilters(
            filter_type=FILTER_TYPE_BASIC,
            start_height=1000,
            stop_hash=int(stale_block_hash, 16),
        )
        peer_0.send_and_ping(request)
        response = peer_0.pop_cfilters()
        assert_equal(len(response), 1)

        cfilter = response[0]
        assert_equal(cfilter.filter_type, FILTER_TYPE_BASIC)
        assert_equal(cfilter.block_hash, int(stale_block_hash, 16))
        computed_cfhash = uint256_from_str(hash256(cfilter.filter_data))
        assert_equal(computed_cfhash, stale_cfhashes[999])

        self.log.info("Requests to node 1 without NODE_COMPACT_FILTERS results in disconnection.")
        requests = [
            msg_getcfcheckpt(
                filter_type=FILTER_TYPE_BASIC,
                stop_hash=int(main_block_hash, 16),
            ),
            msg_getcfheaders(
                filter_type=FILTER_TYPE_BASIC,
                start_height=1000,
                stop_hash=int(main_block_hash, 16),
            ),
            msg_getcfilters(
                filter_type=FILTER_TYPE_BASIC,
                start_height=1000,
                stop_hash=int(main_block_hash, 16),
            ),
        ]
        for request in requests:
            peer_1 = self.nodes[1].add_p2p_connection(P2PInterface())
            peer_1.send_message(request)
            peer_1.wait_for_disconnect()

        self.log.info("Check that invalid requests result in disconnection.")
        requests = [
            # Requesting too many filters results in disconnection.
            msg_getcfilters(
                filter_type=FILTER_TYPE_BASIC,
                start_height=0,
                stop_hash=int(main_block_hash, 16),
            ),
            # Requesting too many filter headers results in disconnection.
            msg_getcfheaders(
                filter_type=FILTER_TYPE_BASIC,
                start_height=0,
                stop_hash=int(tip_hash, 16),
            ),
            # Requesting unknown filter type results in disconnection.
            msg_getcfcheckpt(
                filter_type=255,
                stop_hash=int(main_block_hash, 16),
            ),
            # Requesting unknown hash results in disconnection.
            msg_getcfcheckpt(
                filter_type=FILTER_TYPE_BASIC,
                stop_hash=123456789,
            ),
        ]
        for request in requests:
            peer_0 = self.nodes[0].add_p2p_connection(P2PInterface())
            peer_0.send_message(request)
            peer_0.wait_for_disconnect()


def compute_last_header(prev_header, hashes):
    """Compute the last filter header from a starting header and a sequence of filter hashes."""
    header = ser_uint256(prev_header)
    for filter_hash in hashes:
        header = hash256(ser_uint256(filter_hash) + header)
    return uint256_from_str(header)


if __name__ == '__main__':
    CompactFiltersTest().main()
