#!/usr/bin/env python3
# Copyright (c) 2022- The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test stalling logic during IBD
"""

import time

from test_framework.blocktools import (
        create_block,
        create_coinbase
)
from test_framework.messages import (
        MSG_BLOCK,
        MSG_TYPE_MASK,
)
from test_framework.p2p import (
        CBlockHeader,
        msg_block,
        msg_headers,
        P2PDataStore,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
        assert_equal,
)


class P2PStaller(P2PDataStore):
    def __init__(self, stall_blocks):
        self.stall_blocks = stall_blocks
        super().__init__()

    def on_getdata(self, message):
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) == MSG_BLOCK:
                if (inv.hash not in self.stall_blocks):
                    self.send_without_ping(msg_block(self.block_store[inv.hash]))

    def on_getheaders(self, message):
        pass


class P2PIBDStallingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        NUM_BLOCKS = 1025
        NUM_PEERS = 5
        node = self.nodes[0]
        tip = int(node.getbestblockhash(), 16)
        blocks = []
        height = 1
        block_time = node.getblock(node.getbestblockhash())['time'] + 1
        self.log.info("Prepare blocks without sending them to the node")
        block_dict = {}
        for _ in range(NUM_BLOCKS):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].hash_int
            block_time += 1
            height += 1
            block_dict[blocks[-1].hash_int] = blocks[-1]
        stall_index = 0
        second_stall_index = 500
        stall_blocks = [blocks[stall_index].hash_int, blocks[second_stall_index].hash_int]

        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in blocks[:NUM_BLOCKS-1]]
        peers = []

        self.log.info("Check that a staller does not get disconnected if the 1024 block lookahead buffer is filled")
        self.mocktime = int(time.time()) + 1
        node.setmocktime(self.mocktime)
        for id in range(NUM_PEERS):
            peers.append(node.add_outbound_p2p_connection(P2PStaller(stall_blocks), p2p_idx=id, connection_type="outbound-full-relay"))
            peers[-1].block_store = block_dict
            peers[-1].send_and_ping(headers_message)

        # Wait until all blocks are received (except for the stall blocks), so that no other blocks are in flight.
        self.wait_until(lambda: sum(len(peer['inflight']) for peer in node.getpeerinfo()) == len(stall_blocks))

        self.all_sync_send_with_ping(peers)
        # If there was a peer marked for stalling, it would get disconnected
        self.mocktime += 3
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS)

        self.log.info("Check that increasing the window beyond 1024 blocks triggers stalling logic")
        headers_message.headers = [CBlockHeader(b) for b in blocks]
        with node.assert_debug_log(expected_msgs=['Stall started']):
            for p in peers:
                p.send_without_ping(headers_message)
            self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling peer is disconnected after 2 seconds")
        self.mocktime += 3
        node.setmocktime(self.mocktime)
        peers[0].wait_for_disconnect()
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 1)
        self.wait_until(lambda: self.is_block_requested(peers, stall_blocks[0]))
        # Make sure that SendMessages() is invoked, which assigns the missing block
        # to another peer and starts the stalling logic for them
        self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling timeout gets doubled to 4 seconds for the next staller")
        # No disconnect after just 3 seconds
        self.mocktime += 3
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 1)

        self.mocktime += 2
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(x.is_connected for x in node.p2ps) == NUM_PEERS - 2)
        self.wait_until(lambda: self.is_block_requested(peers, stall_blocks[0]))
        self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling timeout gets doubled to 8 seconds for the next staller")
        # No disconnect after just 7 seconds
        self.mocktime += 7
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 2)

        self.mocktime += 2
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(x.is_connected for x in node.p2ps) == NUM_PEERS - 3)
        self.wait_until(lambda: self.is_block_requested(peers, stall_blocks[0]))
        self.all_sync_send_with_ping(peers)

        self.log.info("Provide the first withheld block and check that stalling timeout gets reduced back to 2 seconds")
        with node.assert_debug_log(expected_msgs=['Decreased stalling timeout to 2 seconds'], unexpected_msgs=['Stall started']):
            for p in peers:
                if p.is_connected and (stall_blocks[0] in p.getdata_requests):
                    p.send_without_ping(msg_block(block_dict[stall_blocks[0]]))
            self.all_sync_send_with_ping(peers)

        self.log.info("Check that all outstanding blocks up to the second stall block get connected")
        self.wait_until(lambda: node.getblockcount() == second_stall_index)


    def all_sync_send_with_ping(self, peers):
        for p in peers:
            if p.is_connected:
                p.sync_with_ping()

    def is_block_requested(self, peers, hash):
        for p in peers:
            if p.is_connected and (hash in p.getdata_requests):
                return True
        return False


if __name__ == '__main__':
    P2PIBDStallingTest(__file__).main()
