#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

""" Test block announcement time tracking

The bitcoind client records, for each peer, the most recent time that
this peer announced a block that the client wasn't already aware of. This
timestamp, CNodeStateStats::m_last_block_announcement, is available in the
`last_block_announcement` field of each peer's `getpeerinfo` result. The
value zero means that this peer has never been the first to announce
a block to us. Blocks are announced using either the "headers" or
"cmpctblock" messages.

This timestamp is used when the "potential stale tip" condition occurs:
When a new block hasn't been seen for a longer-than-expected amount of
time (currently 30 minutes, see TipMayBeStale()), the client, suspecting
that there may be new blocks that its peers are not announcing, will
add an extra outbound peer and disconnect (evict) the peer that has
least recently been the first to announced a new block to us. (If there
is a tie, it will disconnect the most recently-added of those peers.)

This test verifies that this timestamp is being set correctly.
(This tests PR 26172.)
"""

import time
from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    CBlockHeader,
    msg_headers,
)
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework


class P2PBlockTimes(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.disable_autoconnect = False

    def run_test(self):
        node = self.nodes[0]
        cur_time = int(time.time())
        node.setmocktime(cur_time)

        # Generate one block to exit IBD
        self.generate(node, 1)

        self.log.info("Create a full-outbound test framework peer")
        node.add_outbound_p2p_connection(P2PDataStore(), p2p_idx=0)

        self.log.info("Test framework peer generates a new block")
        tip = int(node.getbestblockhash(), 16)
        block = create_block(tip, create_coinbase(2))
        block.solve()

        self.log.info("Test framework peer sends node the new block")
        node.p2ps[0].send_blocks_and_test([block], node, success=True)

        self.log.info("Verify peerinfo block timestamps")
        peerinfo = node.getpeerinfo()[0]
        assert peerinfo['last_block'] == cur_time
        assert peerinfo['last_block_announcement'] == cur_time

        self.log.info("Sending a block announcement with no new blocks")
        node.setmocktime(cur_time+1)
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(block)]
        node.p2ps[0].send_message(headers_message)
        node.p2ps[0].sync_with_ping()

        self.log.info("Verify that block announcement time isn't updated")
        peerinfo = node.getpeerinfo()[0]
        assert peerinfo['last_block_announcement'] == cur_time

if __name__ == '__main__':
    P2PBlockTimes().main()
