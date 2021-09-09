#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test compact blocks HB selection logic."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class CompactBlocksConnectionTest(BitcoinTestFramework):
    """Test class for verifying selection of HB peer connections."""

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 6

    def peer_info(self, from_node, to_node):
        """Query from_node for its getpeerinfo about to_node."""
        for peerinfo in self.nodes[from_node].getpeerinfo():
            if "(testnode%i)" % to_node in peerinfo['subver']:
                return peerinfo
        return None

    def setup_network(self):
        self.setup_nodes()
        # Start network with everyone disconnected
        self.sync_all()

    def relay_block_through(self, peer):
        """Relay a new block through peer peer, and return HB status between 1 and [2,3,4,5]."""
        self.connect_nodes(peer, 0)
        self.generate(self.nodes[0], 1)
        self.sync_blocks()
        self.disconnect_nodes(peer, 0)
        status_to = [self.peer_info(1, i)['bip152_hb_to'] for i in range(2, 6)]
        status_from = [self.peer_info(i, 1)['bip152_hb_from'] for i in range(2, 6)]
        assert_equal(status_to, status_from)
        return status_to

    def run_test(self):
        self.log.info("Testing reserved high-bandwidth mode slot for outbound peer...")

        # Connect everyone to node 0, and mine some blocks to get all nodes out of IBD.
        for i in range(1, 6):
            self.connect_nodes(i, 0)
        self.generate(self.nodes[0], 2)
        self.sync_blocks()
        for i in range(1, 6):
            self.disconnect_nodes(i, 0)

        # Construct network topology:
        # - Node 0 is the block producer
        # - Node 1 is the "target" node being tested
        # - Nodes 2-5 are intermediaries.
        #   - Node 1 has an outbound connection to node 2
        #   - Node 1 has inbound connections from nodes 3-5
        self.connect_nodes(3, 1)
        self.connect_nodes(4, 1)
        self.connect_nodes(5, 1)
        self.connect_nodes(1, 2)

        # Mine blocks subsequently relaying through nodes 3,4,5 (inbound to node 1)
        for nodeid in range(3, 6):
            status = self.relay_block_through(nodeid)
            assert_equal(status, [False, nodeid >= 3, nodeid >= 4, nodeid >= 5])

        # And again through each. This should not change HB status.
        for nodeid in range(3, 6):
            status = self.relay_block_through(nodeid)
            assert_equal(status, [False, True, True, True])

        # Now relay one block through peer 2 (outbound from node 1), so it should take HB status
        # from one of the inbounds.
        status = self.relay_block_through(2)
        assert_equal(status[0], True)
        assert_equal(sum(status), 3)

        # Now relay again through nodes 3,4,5. Since 2 is outbound, it should remain HB.
        for nodeid in range(3, 6):
            status = self.relay_block_through(nodeid)
            assert status[0]
            assert status[nodeid - 2]
            assert_equal(sum(status), 3)

        # Reconnect peer 2, and retry. Now the three inbounds should be HB again.
        self.disconnect_nodes(1, 2)
        self.connect_nodes(1, 2)
        for nodeid in range(3, 6):
            status = self.relay_block_through(nodeid)
            assert not status[0]
            assert status[nodeid - 2]
        assert_equal(status, [False, True, True, True])


if __name__ == '__main__':
    CompactBlocksConnectionTest().main()
