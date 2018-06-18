#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test behavior of -maxuploadtarget.

* Verify that getdata requests for old blocks (>1week) are dropped
if uploadtarget has been reached.
* Verify that getdata requests for recent blocks are respected even
if uploadtarget has been reached.
* Verify that the upload counters are reset after 24 hours.
"""
from collections import defaultdict
import time

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.block_receive_map = defaultdict(int)

    def on_inv(self, message):
        pass

    def on_block(self, message):
        message.block.calc_sha256()
        self.block_receive_map[message.block.sha256] += 1

class MaxUploadTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-maxuploadtarget=800"]]

        # Cache for utxos, as the listunspent may take a long time later in the test
        self.utxo_cache = []

    def run_test(self):
        # Before we connect anything, we first set the time on the node
        # to be in the past, otherwise things break because the CNode
        # time counters can't be reset backward after initialization
        old_time = int(time.time() - 2*60*60*24*7)
        self.nodes[0].setmocktime(old_time)

        # Generate some old blocks
        self.nodes[0].generate(130)

        # p2p_conns[0] will only request old blocks
        # p2p_conns[1] will only request new blocks
        # p2p_conns[2] will test resetting the counters
        p2p_conns = []

        for _ in range(3):
            p2p_conns.append(self.nodes[0].add_p2p_connection(TestP2PConn()))

        for p2pc in p2p_conns:
            p2pc.wait_for_verack()

        # Test logic begins here

        # Now mine a big block
        mine_large_block(self.nodes[0], self.utxo_cache)

        # Store the hash; we'll request this later
        big_old_block = self.nodes[0].getbestblockhash()
        old_block_size = self.nodes[0].getblock(big_old_block, True)['size']
        big_old_block = int(big_old_block, 16)

        # Advance to two days ago
        self.nodes[0].setmocktime(int(time.time()) - 2*60*60*24)

        # Mine one more block, so that the prior block looks old
        mine_large_block(self.nodes[0], self.utxo_cache)

        # We'll be requesting this new block too
        big_new_block = self.nodes[0].getbestblockhash()
        big_new_block = int(big_new_block, 16)

        # p2p_conns[0] will test what happens if we just keep requesting the
        # the same big old block too many times (expect: disconnect)

        getdata_request = msg_getdata()
        getdata_request.inv.append(CInv(2, big_old_block))

        max_bytes_per_day = 800*1024*1024
        daily_buffer = 144 * 4000000
        max_bytes_available = max_bytes_per_day - daily_buffer
        success_count = max_bytes_available // old_block_size

        # 576MB will be reserved for relaying new blocks, so expect this to
        # succeed for ~235 tries.
        for i in range(success_count):
            p2p_conns[0].send_message(getdata_request)
            p2p_conns[0].sync_with_ping()
            assert_equal(p2p_conns[0].block_receive_map[big_old_block], i+1)

        assert_equal(len(self.nodes[0].getpeerinfo()), 3)
        # At most a couple more tries should succeed (depending on how long
        # the test has been running so far).
        for i in range(3):
            p2p_conns[0].send_message(getdata_request)
        p2p_conns[0].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 2)
        self.log.info("Peer 0 disconnected after downloading old block too many times")

        # Requesting the current block on p2p_conns[1] should succeed indefinitely,
        # even when over the max upload target.
        # We'll try 800 times
        getdata_request.inv = [CInv(2, big_new_block)]
        for i in range(800):
            p2p_conns[1].send_message(getdata_request)
            p2p_conns[1].sync_with_ping()
            assert_equal(p2p_conns[1].block_receive_map[big_new_block], i+1)

        self.log.info("Peer 1 able to repeatedly download new block")

        # But if p2p_conns[1] tries for an old block, it gets disconnected too.
        getdata_request.inv = [CInv(2, big_old_block)]
        p2p_conns[1].send_message(getdata_request)
        p2p_conns[1].wait_for_disconnect()
        assert_equal(len(self.nodes[0].getpeerinfo()), 1)

        self.log.info("Peer 1 disconnected after trying to download old block")

        self.log.info("Advancing system time on node to clear counters...")

        # If we advance the time by 24 hours, then the counters should reset,
        # and p2p_conns[2] should be able to retrieve the old block.
        self.nodes[0].setmocktime(int(time.time()))
        p2p_conns[2].sync_with_ping()
        p2p_conns[2].send_message(getdata_request)
        p2p_conns[2].sync_with_ping()
        assert_equal(p2p_conns[2].block_receive_map[big_old_block], 1)

        self.log.info("Peer 2 able to download old block")

        self.nodes[0].disconnect_p2ps()

        #stop and start node 0 with 1MB maxuploadtarget, whitelist 127.0.0.1
        self.log.info("Restarting nodes with -whitelist=127.0.0.1")
        self.stop_node(0)
        self.start_node(0, ["-whitelist=127.0.0.1", "-maxuploadtarget=1"])

        # Reconnect to self.nodes[0]
        self.nodes[0].add_p2p_connection(TestP2PConn())
        self.nodes[0].p2p.wait_for_verack()

        #retrieve 20 blocks which should be enough to break the 1MB limit
        getdata_request.inv = [CInv(2, big_new_block)]
        for i in range(20):
            self.nodes[0].p2p.send_message(getdata_request)
            self.nodes[0].p2p.sync_with_ping()
            assert_equal(self.nodes[0].p2p.block_receive_map[big_new_block], i+1)

        getdata_request.inv = [CInv(2, big_old_block)]
        self.nodes[0].p2p.send_and_ping(getdata_request)
        assert_equal(len(self.nodes[0].getpeerinfo()), 1) #node is still connected because of the whitelist

        self.log.info("Peer still connected after trying to download old block (whitelisted)")

if __name__ == '__main__':
    MaxUploadTest().main()
