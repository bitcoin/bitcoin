#!/usr/bin/env python3
# Copyright (c) 2024- The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test SyncCoinsTipAfterChainSync logic
"""


from test_framework.blocktools import create_block, create_coinbase
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


class P2PBlockDelay(P2PDataStore):
    def __init__(self, delay_block):
        self.delay_block = delay_block
        super().__init__()

    def on_getdata(self, message):
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) == MSG_BLOCK:
                if inv.hash != self.delay_block:
                    self.send_message(msg_block(self.block_store[inv.hash]))

    def on_getheaders(self, message):
        pass

    def send_delayed(self):
        self.send_message(msg_block(self.block_store[self.delay_block]))


SYNC_CHECK_INTERVAL = 30


class SyncCoinsTipAfterChainSyncTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        # Set maxtipage to 1 to get us out of IBD after 1 block past our mocktime
        self.extra_args = [[f"-maxtipage=1"]]

    def run_test(self):
        NUM_BLOCKS = 3
        node = self.nodes[0]
        tip = int(node.getbestblockhash(), 16)
        blocks = []
        height = 1
        block_time = node.getblock(node.getbestblockhash())["time"] + 1
        # Set mock time to 2 past block time, so second block will exit IBD
        node.setmocktime(block_time + 2)

        # Prepare blocks without sending them to the node
        block_dict = {}
        for _ in range(NUM_BLOCKS):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1
            block_dict[blocks[-1].sha256] = blocks[-1]
        delay_block = blocks[-1].sha256

        # Create peer which will not automatically send last block
        peer = node.add_outbound_p2p_connection(
            P2PBlockDelay(delay_block),
            p2p_idx=1,
            connection_type="outbound-full-relay",
        )
        peer.block_store = block_dict

        self.log.info(
            "Send headers message for first block, verify it won't sync because node is still in IBD"
        )
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(blocks[0])]
        peer.send_message(headers_message)
        peer.sync_with_ping()
        assert_equal(node.getblockchaininfo()["initialblockdownload"], True)
        with node.assert_debug_log(
            ["Node is still in IBD, rescheduling post-IBD chainstate disk sync..."]
        ):
            node.mockscheduler(SYNC_CHECK_INTERVAL)

        self.log.info(
            "Send headers message for second block, verify it won't sync because node height has changed"
        )
        headers_message.headers = [CBlockHeader(blocks[1])]
        peer.send_message(headers_message)
        peer.sync_with_ping()
        assert_equal(node.getblockchaininfo()["initialblockdownload"], False)
        with node.assert_debug_log(
            [
                "Chain height updated since last check, rescheduling post-IBD chainstate disk sync..."
            ]
        ):
            node.mockscheduler(SYNC_CHECK_INTERVAL)

        self.log.info(
            "Send headers message for last block, verify it won't sync because node is still downloading the block"
        )
        headers_message.headers = [CBlockHeader(blocks[2])]
        peer.send_message(headers_message)
        peer.sync_with_ping()
        with node.assert_debug_log(
            [
                "Still downloading blocks from peers, rescheduling post-IBD chainstate disk sync..."
            ]
        ):
            node.mockscheduler(SYNC_CHECK_INTERVAL)

        self.log.info(
            "Send last block, verify it won't sync because node height has changed"
        )
        peer.send_delayed()
        peer.sync_with_ping()
        with node.assert_debug_log(
            [
                "Chain height updated since last check, rescheduling post-IBD chainstate disk sync..."
            ]
        ):
            node.mockscheduler(SYNC_CHECK_INTERVAL)

        self.log.info("Verify node syncs chainstate to disk on next scheduler update")
        with node.assert_debug_log(
            ["Finished syncing to tip, syncing chainstate to disk"]
        ):
            node.mockscheduler(SYNC_CHECK_INTERVAL)


if __name__ == "__main__":
    SyncCoinsTipAfterChainSyncTest().main()
