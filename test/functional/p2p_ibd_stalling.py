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
        COutPoint,
        CTransaction,
        CTxIn,
        CTxOut,
        HeaderAndShortIDs,
        MSG_BLOCK,
        MSG_TYPE_MASK,
        msg_cmpctblock,
        msg_sendcmpct,
)
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.p2p import (
        CBlockHeader,
        msg_block,
        msg_headers,
        P2PDataStore,
        p2p_lock,
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
        self.num_nodes = 3

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
        block_time = int(time.time())
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
        self.mocktime = int(time.time()) + 1
        for id in range(NUM_PEERS):
            peers.append(node.add_outbound_p2p_connection(P2PStaller(stall_block), p2p_idx=id, connection_type="outbound-full-relay"))
            peers[-1].block_store = self.block_dict
            peers[-1].send_message(headers_message)

        # Wait until all blocks are received (except for stall_block), so that no other blocks are in flight.
        self.wait_until(lambda: sum(len(peer['inflight']) for peer in node.getpeerinfo()) == 1)
        self.all_sync_send_with_ping(peers)
        # If there was a peer marked for stalling, it would get disconnected
        self.mocktime += 3
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
        self.log.info("Part 3: Test stalling close to the tip")
        # only send <= 1024 headers, so that the window can't overshoot and the ibd stalling mechanism isn't triggered
        # make sure it works at different lengths
        for header_length in [1, 10, 1024]:
            peers = []
            stall_block = self.blocks[0].sha256
            headers_message = msg_headers()
            headers_message.headers = [CBlockHeader(b) for b in self.blocks[:self.NUM_BLOCKS-1][:header_length]]

            self.mocktime = int(time.time())
            node.setmocktime(self.mocktime)

            self.log.info(f"Add three stalling peers, sending {header_length} headers")
            for id in range(4):
                peers.append(node.add_outbound_p2p_connection(P2PStaller(stall_block), p2p_idx=id, connection_type="outbound-full-relay"))
                peers[-1].block_store = self.block_dict
                peers[-1].send_message(headers_message)

            self.wait_until(lambda: sum(len(peer['inflight']) for peer in node.getpeerinfo()) == 1)
            self.all_sync_send_with_ping(peers)
            assert_equal(sum(peer.stall_block_requested for peer in peers),  1)

            self.log.info("Check that after 30 seconds we request the block from a second peer")
            self.mocktime += 31
            node.setmocktime(self.mocktime)
            self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 2)

            self.log.info("Check that after another 30 seconds we request the block from a third peer")
            self.mocktime += 31
            node.setmocktime(self.mocktime)
            self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 3)

            self.log.info("Check that after another 30 seconds we aren't requesting it from a fourth peer yet")
            self.mocktime += 31
            node.setmocktime(self.mocktime)
            self.all_sync_send_with_ping(peers)
            self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 3)

            self.log.info("Check that after another 20 minutes, first three stalling peers are disconnected")
            # 10 minutes BLOCK_DOWNLOAD_TIMEOUT_BASE + 2*5 minutes BLOCK_DOWNLOAD_TIMEOUT_PER_PEER
            self.mocktime += 20 * 60
            node.setmocktime(self.mocktime)
            # all peers have been requested
            self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 4)

            self.log.info("Check that after another 20 minutes, last stalling peer is disconnected")
            # 10 minutes BLOCK_DOWNLOAD_TIMEOUT_BASE + 2*5 minutes BLOCK_DOWNLOAD_TIMEOUT_PER_PEER
            self.mocktime += 20 * 60
            node.setmocktime(self.mocktime)
            for peer in peers:
                peer.wait_for_disconnect()

        self.log.info("Provide missing block and check that the sync succeeds")
        peer = node.add_outbound_p2p_connection(P2PStaller(stall_block), p2p_idx=0, connection_type="outbound-full-relay")
        peer.send_message(msg_block(self.block_dict[stall_block]))
        self.wait_until(lambda: node.getblockcount() == self.NUM_BLOCKS - 1)
        node.disconnect_p2ps()

    def at_tip_stalling(self):
        self.log.info("Test stalling and interaction with compact blocks when at tip")
        node = self.nodes[2]
        peers = []
        # Create a block with a tx (would be invalid, but this doesn't matter since we will only ever send the header)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.blocks[1].vtx[0].sha256, 0), scriptSig=b""))
        tx.vout.append(CTxOut(49 * 100000000, CScript([OP_TRUE])))
        tx.calc_sha256()
        block_time = self.blocks[1].nTime + 1
        block = create_block(self.blocks[1].sha256, create_coinbase(3), block_time, txlist=[tx])
        block.solve()

        for id in range(3):
            peers.append(node.add_outbound_p2p_connection(P2PStaller(block.sha256), p2p_idx=id, connection_type="outbound-full-relay"))

        # First Peer is a high-bw compact block peer
        peers[0].send_and_ping(msg_sendcmpct(announce=True, version=2))
        peers[0].block_store = self.block_dict
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in self.blocks[:2]]
        peers[0].send_message(headers_message)
        self.wait_until(lambda: node.getblockcount() == 2)

        self.log.info("First peer announces via cmpctblock")
        cmpct_block = HeaderAndShortIDs()
        cmpct_block.initialize_from_block(block)
        peers[0].send_and_ping(msg_cmpctblock(cmpct_block.to_p2p()))
        with p2p_lock:
            assert "getblocktxn" in peers[0].last_message

        self.log.info("Also announce block from other peers by header")
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(block)]
        for peer in peers[1:4]:
            peer.send_and_ping(headers_message)

        self.log.info("Check that block is requested from two more header-announcing peers")
        self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 0)

        self.mocktime = int(time.time()) + 31
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 1)

        self.mocktime += 31
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 2)

        self.log.info("Check that block is not requested from a third header-announcing peer")
        self.mocktime += 31
        node.setmocktime(self.mocktime)
        self.wait_until(lambda: sum(peer.stall_block_requested for peer in peers) == 2)

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
        self.at_tip_stalling()


if __name__ == '__main__':
    P2PIBDStallingTest(__file__).main()
