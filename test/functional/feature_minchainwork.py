#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for setting nMinimumChainWork on command line.

Nodes don't consider themselves out of "initial block download" until
their active chain has more work than nMinimumChainWork.

Nodes don't download blocks from a peer unless the peer's best known block
has more work than nMinimumChainWork.

While in initial block download, nodes won't relay blocks to their peers, so
test that this parameter functions as intended by verifying that block relay
only succeeds past a given node once its nMinimumChainWork has been exceeded.

While in initial block download, nodes disconnect outbound (full-relay and
block-relay-only) peers whose headers chain has less work than
nMinimumChainWork.
"""

import time

from test_framework.messages import (
    CBlockHeader,
    from_hex,
    msg_headers,
)
from test_framework.p2p import P2PInterface, msg_getheaders
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
    assert_not_equal,
)

# 2 hashes required per regtest block (with no difficulty adjustment)
REGTEST_WORK_PER_BLOCK = 2

class MinimumChainWorkTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

        self.extra_args = [[], ["-minimumchainwork=0x65"], ["-minimumchainwork=0x65"]]
        self.node_min_work = [0, 101, 101]

    def setup_network(self):
        # This test relies on the chain setup being:
        # node0 <- node1 <- node2
        # Before leaving IBD, nodes prefer to download blocks from outbound
        # peers, so ensure that we're mining on an outbound peer and testing
        # block relay to inbound peers.
        self.setup_nodes()
        for i in range(self.num_nodes-1):
            self.connect_nodes(i+1, i)

        # Set clock of node2 2 days ahead, to keep it in IBD during this test.
        self.nodes[2].setmocktime(int(time.time()) + 48*60*60)

    def test_outbound_insufficient_work_disconnect(self):
        self.log.info("Test that outbound peers serving an insufficient work chain are disconnected during IBD")
        # Restart node2 with a minimum chain work above its current chain, so
        # that it stays in IBD and its own chain has insufficient work.
        self.restart_node(2, extra_args=["-minimumchainwork=0x1000"])
        node = self.nodes[2]
        assert_equal(node.getblockchaininfo()['initialblockdownload'], True)
        # Headers already in node2's block index (ancestors of its tip) skip the
        # low-work headers sync, so they reach the insufficient work check.
        headers = [from_hex(CBlockHeader(), node.getblockheader(node.getblockhash(height), False))
                   for height in range(1, node.getblockcount() + 1)]
        self.log.info("Check that inbound and manual peers are not disconnected")
        with node.assert_debug_log(expected_msgs=[], unexpected_msgs=["outbound peer headers chain has insufficient work"]):
            inbound_peer = node.add_p2p_connection(P2PInterface())
            manual_peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="manual")
            for peer in [inbound_peer, manual_peer]:
                peer.send_and_ping(msg_headers(headers))
                assert peer.is_connected
        node.disconnect_p2ps()
        for p2p_idx, conn_type in enumerate(["outbound-full-relay", "block-relay-only"]):
            self.log.info(f"Check that the {conn_type} peer is disconnected")
            peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=p2p_idx, connection_type=conn_type)
            with node.assert_debug_log(["outbound peer headers chain has insufficient work"]):
                peer.send_without_ping(msg_headers(headers))
                peer.wait_for_disconnect()

    def run_test(self):
        # Start building a chain on node0.  node2 shouldn't be able to sync until node1's
        # minchainwork is exceeded
        starting_chain_work = REGTEST_WORK_PER_BLOCK # Genesis block's work
        self.log.info(f"Testing relay across node 1 (minChainWork = {self.node_min_work[1]})")

        starting_blockcount = self.nodes[2].getblockcount()

        num_blocks_to_generate = int((self.node_min_work[1] - starting_chain_work) / REGTEST_WORK_PER_BLOCK)
        self.log.info(f"Generating {num_blocks_to_generate} blocks on node0")
        hashes = self.generate(self.nodes[0], num_blocks_to_generate, sync_fun=self.no_op)

        self.log.info(f"Node0 current chain work: {self.nodes[0].getblockheader(hashes[-1])['chainwork']}")
        self.log.info("Verifying node 2 has no more blocks than before")
        self.log.info(f"Blockcounts: {[n.getblockcount() for n in self.nodes]}")
        # Node2 shouldn't have any new headers yet, because node1 should not
        # have relayed anything.
        # We wait 3 seconds, rather than sync_blocks(node0, node1) because
        # it's reasonable either way for node1 to get the blocks, or not get
        # them (since they're below node1's minchainwork).
        ensure_for(duration=3, f=lambda: len(self.nodes[2].getchaintips()) == 1)
        assert_equal(self.nodes[2].getchaintips()[0]['height'], 0)

        assert_not_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(self.nodes[2].getblockcount(), starting_blockcount)

        self.log.info("Check that getheaders requests to node2 are ignored")
        peer = self.nodes[2].add_p2p_connection(P2PInterface())
        msg = msg_getheaders()
        msg.locator.vHave = [int(self.nodes[2].getbestblockhash(), 16)]
        msg.hashstop = 0
        peer.send_and_ping(msg)
        ensure_for(duration=5, f=lambda: "headers" not in peer.last_message or len(peer.last_message["headers"].headers) == 0)

        self.log.info("Generating one more block")
        self.generate(self.nodes[0], 1)

        self.log.info("Verifying nodes are all synced")

        # Because nodes in regtest are all manual connections (eg using
        # addnode), node1 should not have disconnected node0. If not for that,
        # we'd expect node1 to have disconnected node0 for serving an
        # insufficient work chain, in which case we'd need to reconnect them to
        # continue the test.

        self.sync_all()
        self.log.info(f"Blockcounts: {[n.getblockcount() for n in self.nodes]}")

        self.log.info("Test that getheaders requests to node2 are not ignored")
        peer.send_and_ping(msg)
        assert "headers" in peer.last_message

        # Verify that node2 is in fact still in IBD (otherwise this test may
        # not be exercising the logic we want!)
        assert_equal(self.nodes[2].getblockchaininfo()['initialblockdownload'], True)

        self.log.info("Test -minimumchainwork with a non-hex value")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            ["-minimumchainwork=test"],
            expected_msg='Error: Invalid minimum work specified (test), must be up to 64 hex digits',
        )

        self.test_outbound_insufficient_work_disconnect()


if __name__ == '__main__':
    MinimumChainWorkTest(__file__).main()
