#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that we reject low difficulty headers to prevent our block tree from filling up with useless bloat"""

from test_framework.test_framework import BitcoinTestFramework

from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)

from test_framework.messages import (
    CBlockHeader,
    msg_headers,
)

from test_framework.blocktools import (
    MAX_FUTURE_BLOCK_TIME,
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)

from test_framework.util import assert_equal

import re
import time

NODE1_BLOCKS_REQUIRED = 15
NODE2_BLOCKS_REQUIRED = 2047
MAX_HEADERS_RESULTS = 2000
REGTEST_TARGET = 0x7fffff << (8 * (0x20 - 3))


class HeadersServer(P2PInterface):
    def __init__(self):
        super().__init__()
        self.getheaders_count = 0
        self.last_locator = []

    def on_getheaders(self, message):
        self.getheaders_count += 1
        self.last_locator = message.locator.vHave


class RejectLowDifficultyHeadersTest(BitcoinTestFramework):
    def set_test_params(self):
        self.rpc_timeout *= 4  # To avoid timeout when generating BLOCKS_TO_MINE
        self.setup_clean_chain = True
        self.num_nodes = 4
        # Node0 has no required chainwork; node1 requires 15 blocks on top of the genesis block; node2 requires 2047
        self.extra_args = [["-minimumchainwork=0x0", "-checkblockindex=0"], ["-minimumchainwork=0x1f", "-checkblockindex=0"], ["-minimumchainwork=0x1000", "-checkblockindex=0"], ["-minimumchainwork=0x1000", "-checkblockindex=0", "-whitelist=noban@127.0.0.1"]]

    def setup_network(self):
        self.setup_nodes()
        self.reconnect_all()
        self.sync_all()

    def disconnect_all(self):
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(0, 2)
        self.disconnect_nodes(0, 3)

    def reconnect_all(self):
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.connect_nodes(0, 3)

    def mocktime_all(self, time):
        for n in self.nodes:
            n.setmocktime(time)

    def test_presync_peer_not_evicted(self):
        self.log.info("Test that an outbound peer serving presync headers isn't evicted by the chain sync timeout")
        self.disconnect_all()
        # Each regtest header adds 2 work, so this needs 65536 headers. Serving
        # one full headers message per minute for 25 minutes is longer than
        # CHAIN_SYNC_TIMEOUT + HEADERS_RESPONSE_TIME, but doesn't finish presync.
        num_msgs = 25
        self.restart_node(1, extra_args=["-minimumchainwork=0x20000", "-checkblockindex=0"])
        node = self.nodes[1]
        genesis = node.getblockheader(node.getblockhash(0))
        headers = []
        prev_hash = int(genesis["hash"], 16)
        for i in range(MAX_HEADERS_RESULTS * num_msgs):
            header = CBlockHeader()
            header.nVersion = 4
            header.hashPrevBlock = prev_hash
            header.nTime = genesis["time"] + i + 1
            header.nBits = 0x207fffff
            while header.hash_int > REGTEST_TARGET:
                header.nNonce += 1
            prev_hash = header.hash_int
            headers.append(header)
        position = {header.hash_int: i for i, header in enumerate(headers)}

        now = int(time.time())
        node.setmocktime(now)
        peer = node.add_outbound_p2p_connection(HeadersServer(), p2p_idx=0, connection_type="outbound-full-relay")
        peer.wait_until(lambda: peer.getheaders_count >= 1)
        for _ in range(num_msgs):
            with p2p_lock:
                getheaders_count = peer.getheaders_count
                locator = peer.last_locator
            start = next((position[h] + 1 for h in locator if h in position), 0)
            peer.send_without_ping(msg_headers(headers[start:start + MAX_HEADERS_RESULTS]))
            peer.wait_until(lambda: peer.getheaders_count > getheaders_count or not peer.is_connected)
            assert peer.is_connected
            now += 60
            node.setmocktime(now)
            peer.sync_with_ping()

        info = node.getpeerinfo()[0]
        assert_equal(info["presynced_headers"], MAX_HEADERS_RESULTS * num_msgs)
        assert_equal(info["synced_headers"], -1)
        peer.sync_with_ping()
        assert peer.is_connected

        self.restart_node(1, extra_args=self.extra_args[1])
        self.reconnect_all()

    def test_chains_sync_when_long_enough(self):
        self.log.info("Generate blocks on the node with no required chainwork, and verify nodes 1 and 2 have no new headers in their headers tree")
        with (
                self.nodes[1].assert_debug_log(expected_msgs=["[net] Ignoring low-work chain (height=14)"], timeout=2),
                self.nodes[2].assert_debug_log(expected_msgs=["[net] Ignoring low-work chain (height=14)"], timeout=2),
                self.nodes[3].assert_debug_log(expected_msgs=["Synchronizing blockheaders, height: 14"], timeout=2),
        ):
            self.generate(self.nodes[0], NODE1_BLOCKS_REQUIRED-1, sync_fun=self.no_op)

        # Node3 should always allow headers due to noban permissions
        self.log.info("Check that node3 will sync headers (due to noban permissions)")

        def check_node3_chaintips(num_tips, tip_hash, height):
            node3_chaintips = self.nodes[3].getchaintips()
            assert_equal(len(node3_chaintips), num_tips)
            assert {
                'height': height,
                'hash': tip_hash,
                'branchlen': height,
                'status': 'headers-only',
            } in node3_chaintips

        check_node3_chaintips(2, self.nodes[0].getbestblockhash(), NODE1_BLOCKS_REQUIRED-1)

        for node in self.nodes[1:3]:
            chaintips = node.getchaintips()
            assert_equal(len(chaintips), 1)
            assert {
                'height': 0,
                'hash': '0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206',
                'branchlen': 0,
                'status': 'active',
            } in chaintips

        self.log.info("Generate more blocks to satisfy node1's minchainwork requirement, and verify node2 still has no new headers in headers tree")
        with (
                self.nodes[2].assert_debug_log(expected_msgs=["[net] Ignoring low-work chain (height=15)"], timeout=2),
                self.nodes[3].assert_debug_log(expected_msgs=["Synchronizing blockheaders, height: 15"], timeout=2),
        ):
            self.generate(self.nodes[0], NODE1_BLOCKS_REQUIRED - self.nodes[0].getblockcount(), sync_fun=self.no_op)
        self.sync_blocks(self.nodes[0:2]) # node3 will sync headers (noban permissions) but not blocks (due to minchainwork)

        assert {
            'height': 0,
            'hash': '0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206',
            'branchlen': 0,
            'status': 'active',
        } in self.nodes[2].getchaintips()

        assert_equal(len(self.nodes[2].getchaintips()), 1)

        self.log.info("Check that node3 accepted these headers as well")
        check_node3_chaintips(2, self.nodes[0].getbestblockhash(), NODE1_BLOCKS_REQUIRED)

        self.log.info("Generate long chain for node0/node1/node3")
        self.generate(self.nodes[0], NODE2_BLOCKS_REQUIRED-self.nodes[0].getblockcount(), sync_fun=self.no_op)

        self.log.info("Verify that node2 and node3 will sync the chain when it gets long enough")
        self.sync_blocks()

    def test_peerinfo_includes_headers_presync_height(self):
        self.log.info("Test that getpeerinfo() includes headers presync height")

        # Disconnect network, so that we can find our own peer connection more
        # easily
        self.disconnect_all()

        p2p = self.nodes[0].add_p2p_connection(P2PInterface())
        node = self.nodes[0]

        # Ensure we have a long chain already
        current_height = self.nodes[0].getblockcount()
        if (current_height < 3000):
            self.generate(node, 3000-current_height, sync_fun=self.no_op)

        # Send a group of 2000 headers, forking from genesis.
        new_blocks = []
        hashPrevBlock = int(node.getblockhash(0), 16)
        for i in range(2000):
            block = create_block(hashprev = hashPrevBlock, tmpl=node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
            block.solve()
            new_blocks.append(block)
            hashPrevBlock = block.hash_int

        headers_message = msg_headers(headers=new_blocks)
        p2p.send_and_ping(headers_message)

        # getpeerinfo should show a sync in progress
        assert_equal(node.getpeerinfo()[0]['presynced_headers'], 2000)

        self.log.info("Test whether a lagging clock aborts low-work headers sync")
        node.disconnect_p2ps()
        node.setmocktime(node.getblockheader(node.getblockhash(0))['mediantime'] - MAX_FUTURE_BLOCK_TIME - 1)
        p2p = node.add_p2p_connection(P2PInterface())
        p2p.send_without_ping(headers_message)
        node.wait_until_stopped(expect_error=True, expected_ret_code=[-6,          # Unix
                                                                      3,           # Windows native
                                                                      0xC0000409], # Windows cross builds
                                expected_stderr=re.compile("Failure when attempting to initiate headers sync: System clock"))

    def test_large_reorgs_can_succeed(self):
        self.log.info("Test that a 2000+ block reorg, starting from a point that is more than 2000 blocks before a locator entry, can succeed")

        self.sync_all() # Ensure all nodes are synced.
        self.disconnect_all()

        # locator(block at height T) will have heights:
        # [T, T-1, ..., T-10, T-12, T-16, T-24, T-40, T-72, T-136, T-264,
        #  T-520, T-1032, T-2056, T-4104, ...]
        # So mine a number of blocks > 4104 to ensure that the first window of
        # received headers during a sync are fully between locator entries.
        BLOCKS_TO_MINE = 4110

        self.generate(self.nodes[0], BLOCKS_TO_MINE, sync_fun=self.no_op)
        self.generate(self.nodes[1], BLOCKS_TO_MINE+2, sync_fun=self.no_op)

        self.reconnect_all()

        self.mocktime_all(int(time.time()))  # Temporarily hold time to avoid internal timeouts
        self.sync_blocks(timeout=300) # Ensure tips eventually agree
        self.mocktime_all(0)


    def run_test(self):
        self.test_presync_peer_not_evicted()

        self.test_chains_sync_when_long_enough()

        self.test_large_reorgs_can_succeed()

        self.test_peerinfo_includes_headers_presync_height()



if __name__ == '__main__':
    RejectLowDifficultyHeadersTest(__file__).main()
