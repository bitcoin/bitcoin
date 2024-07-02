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
    def __init__(self, stall_block):
        self.stall_block = stall_block
        self.stall_block_requested = False
        super().__init__()

    def on_getdata(self, message):
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) == MSG_BLOCK:
                if (inv.hash != self.stall_block):
                    self.send_message(msg_block(self.block_store[inv.hash]))
                else:
                    self.stall_block_requested = True

    def on_getheaders(self, message):
        pass


class P2PIBDStallingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.setup_nodes()
        # Don't connect the nodes

    def prepare_blocks(self):
        self.log.info("Prepare blocks without sending them to any node")
        self.NUM_BLOCKS = 1025
        self.block_dict = {}
        self.blocks = []

        node = self.nodes[0]
        tip = int(node.getbestblockhash(), 16)
        height = 1
        block_time = node.getblock(node.getbestblockhash())['time'] + 1
        for _ in range(self.NUM_BLOCKS):
            self.blocks.append(create_block(tip, create_coinbase(height), block_time))
            self.blocks[-1].solve()
            tip = self.blocks[-1].sha256
            block_time += 1
            height += 1
            self.block_dict[self.blocks[-1].sha256] = self.blocks[-1]

    def ibd_stalling(self):
        NUM_PEERS = 4
        stall_block = self.blocks[0].sha256
        node = self.nodes[0]

        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in self.blocks[:self.NUM_BLOCKS-1]]
        peers = []

        self.log.info("Part 1: Test stalling during IBD")
        self.log.info("Check that a staller does not get disconnected if the 1024 block lookahead buffer is filled")
        for id in range(NUM_PEERS):
            peers.append(node.add_outbound_p2p_connection(P2PStaller(stall_block), p2p_idx=id, connection_type="outbound-full-relay"))
            peers[-1].block_store = self.block_dict
            peers[-1].send_message(headers_message)

        # Wait until all blocks are received (except for stall_block), so that no other blocks are in flight.
        self.wait_until(lambda: sum(len(peer['inflight']) for peer in node.getpeerinfo()) == 1)
        self.all_sync_send_with_ping(peers)
        # If there was a peer marked for stalling, it would get disconnected
        self.mocktime = int(time.time()) + 3
        node.setmocktime(self.mocktime)
        self.all_sync_send_with_ping(peers)
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS)

        self.log.info("Check that increasing the window beyond 1024 blocks triggers stalling logic")
        headers_message.headers = [CBlockHeader(b) for b in self.blocks]
        with node.assert_debug_log(expected_msgs=['Stall started']):
            for p in peers:
                p.send_message(headers_message)
            self.all_sync_send_with_ping(peers)

        self.log.info("Check that the stalling peer is disconnected after 2 seconds")
        self.mocktime += 3
        node.setmocktime(self.mocktime)
        peers[0].wait_for_disconnect()
        assert_equal(node.num_test_p2p_connections(), NUM_PEERS - 1)
        self.wait_until(lambda: self.is_block_requested(peers, stall_block))
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
        self.wait_until(lambda: self.is_block_requested(peers, stall_block))
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
        self.wait_until(lambda: self.is_block_requested(peers, stall_block))
        self.all_sync_send_with_ping(peers)

        self.log.info("Provide the withheld block and check that stalling timeout gets reduced back to 2 seconds")
        with node.assert_debug_log(expected_msgs=['Decreased stalling timeout to 2 seconds']):
            for p in peers:
                if p.is_connected and (stall_block in p.getdata_requests):
                    p.send_message(msg_block(self.block_dict[stall_block]))

        self.log.info("Check that all outstanding blocks get connected")
        self.wait_until(lambda: node.getblockcount() == self.NUM_BLOCKS)

    def near_tip_stalling(self):
        node = self.nodes[1]
        peers = []
        stall_block = self.blocks[0].sha256
        self.log.info("Part 2: Test stalling close to the tip")
        # only send 1024 headers, so that the window can't overshoot and the ibd stalling mechanism isn't triggered
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in self.blocks[:self.NUM_BLOCKS-1]]
        self.log.info("Add three stalling peers")
        for id in range(3):
            peers.append(node.add_outbound_p2p_connection(P2PStaller(stall_block), p2p_idx=id, connection_type="outbound-full-relay"))
            peers[-1].block_store = self.block_dict
            peers[-1].send_message(headers_message)

        self.wait_until(lambda: sum(len(peer['inflight']) for peer in node.getpeerinfo()) == 1)
        assert_equal(sum(peer.stall_block_requested for peer in peers),  1)

        self.log.info("Check that after 30 seconds we request the block from a second peer")
        self.mocktime = int(time.time()) + 31
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 2)

        self.log.info("Check that after another 30 seconds we request the block from a third peer")
        self.mocktime += 31
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 3)

        self.log.info("Check that after another 20 minutes, all stalling peers are disconnected")
        # 10 minutes BLOCK_DOWNLOAD_TIMEOUT_BASE + 2*5 minutes BLOCK_DOWNLOAD_TIMEOUT_PER_PEER
        self.mocktime += 20 * 60
        node.setmocktime(self.mocktime)
        for peer in peers:
            peer.wait_for_disconnect()

    def total_bytes_recv_for_blocks(self, node):
        total = 0
        for info in node.getpeerinfo():
            if ("block" in info["bytesrecv_per_msg"].keys()):
                total += info["bytesrecv_per_msg"]["block"]
        return total

    def all_sync_send_with_ping(self, peers):
        for p in peers:
            if p.is_connected:
                p.sync_with_ping()

    def is_block_requested(self, peers, hash):
        for p in peers:
            if p.is_connected and (hash in p.getdata_requests):
                return True
        return False

    def run_test(self):
        self.prepare_blocks()
        self.ibd_stalling()
        self.near_tip_stalling()


if __name__ == '__main__':
    P2PIBDStallingTest().main()
