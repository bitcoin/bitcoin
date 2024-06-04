#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test notfound block responses
"""

from collections import defaultdict

from test_framework.messages import (
    CInv,
    msg_getdata,
    msg_sendnotfound,
    MSG_BLOCK,
    MSG_FILTERED_BLOCK,
    MSG_WITNESS_FLAG,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES
)
from test_framework.protocol import (
    NODE_NETWORK_LIMITED_MIN_BLOCKS
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class NotFoundConnTracker(P2PInterface):
    def __init__(self):
        super().__init__()
        self.not_found_received_map = defaultdict(int)
        self.blocks_received_map = defaultdict(int)

    def send_version(self):
        if self.on_connection_send_msg:
            self.send_message(self.on_connection_send_msg)
            self.on_connection_send_msg = None  # Never used again
            # Activate 'notfound' block response
            self.send_message(msg_sendnotfound())

    def on_inv(self, message):
        pass  # ignore inv messages

    def on_notfound(self, message):
        for inv in message.vec:
            self.not_found_received_map[inv.hash] += 1

    def on_block(self, message):
        message.block.calc_sha256()
        self.blocks_received_map[message.block.sha256] += 1

class NotFoundBlockTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-blocksonly"], ["-prune=1", "-fastprune"]]

    def test_feature_negotiation(self):
        # Initial feature negotiation phase.
        #    a) send SENDNOTFOUND before version. -> output: disconnect.
        #    b) send SENDNOTFOUND after verack. -> output: disconnect.
        #    c) send SENDNOTFOUND in-between version-verack -> output: connected.
        node0 = self.nodes[0]
        self.disconnect_nodes(1, 0)  # isolate node0

        self.log.info("Check disconnection when negotiating 'sendnotfound' before version")
        conn = node0.add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        with node0.assert_debug_log(["Disconnecting peer=1 for sending a feature negotiation message 'sendnotfound' before the version msg"]):
            conn.send_message(msg_sendnotfound())
            conn.wait_for_disconnect()

        self.log.info("Check disconnection when negotiating 'sendnotfound' after verack")
        conn = node0.add_p2p_connection(P2PInterface())
        with node0.assert_debug_log(["'sendnotfound' received after verack from peer=2; disconnecting"]):
            conn.send_message(msg_sendnotfound())
            conn.wait_for_disconnect()

        self.log.info("Check connection succeed when negotiating 'sendnotfound' in-between version-verack handshake")
        conn = node0.add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        conn.peer_connect_send_version(P2P_SERVICES)
        conn.send_version()
        conn.send_message(msg_sendnotfound())
        conn.wait_for_verack()
        conn.sync_with_ping()

        # Ensure message was successfully processed
        info = node0.getpeerinfo()
        assert_equal(len(info), 1)
        assert info[0]['bytessent_per_msg']['sendnotfound'] > 0

        self.connect_nodes(1, 0)  # re-establish connection

    def send_getdata_and_wait_for_notfound(self, node, block_to_request, block_request_type=MSG_BLOCK):
        conn = node.add_p2p_connection(NotFoundConnTracker())
        block_hash = int(block_to_request, 16)
        conn.send_message(msg_getdata([CInv(t=block_request_type, h=block_hash)]))
        self.wait_until(lambda: conn.not_found_received_map[block_hash] == 1 or conn.blocks_received_map[block_hash] == 1)
        assert conn.blocks_received_map.get(block_hash) == 0, " Received block that shouldn't have been sent"

    def test_getdata_response(self):
        # Test 'notfound' for getdata inv requests.
        #    a) send getdata with a random hash. -> response: should receive a notfound.
        #    b) send getdata with a hash of a block from a fork (older than 30 * 24 * 60 * 60). --> response: should receive a not found.
        #    c) send getdata to a prune node for a block deeper than NODE_NETWORK_LIMITED --> response: should receive a not found.
        #    d) send getdata to a prune node for a pruned block --> response: should receive a not found.
        #    e) send getdata (msg_witness type) for a block that is not on disk - need to manually erase it --> response: should receive a not found.
        #    f) send getdata (msg_merkleblock) for a peer that does not have TxRelay enabled -> response: should receive a not found
        self.log.info("Test 'notfound' for getdata block requests..")
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        #######################################################################################################
        self.log.info("(a) Test getdata inv with unknown block hash")
        self.send_getdata_and_wait_for_notfound(node0, block_to_request=b"1")

        #######################################################################################################
        self.log.info("(b) Test getdata inv with a hash of a block from a fork (older than 30 * 24 * 60 * 60)")

        # Disconnect nodes to create two chains
        self.disconnect_nodes(1, 0)
        self.generate(node0, 101, sync_fun=self.no_op)
        self.generate(node1, 20, sync_fun=self.no_op)
        # And obtain fork block hash
        block_to_request = node1.getbestblockhash()

        # Advance time up to the point where the node stops providing blocks from forks
        node_time = node0.getblockheader(node0.getbestblockhash())['time'] + (30 * 24 * 60 * 60) + 1
        for node in [node0, node1]:
            node.setmocktime(node_time)

        # Sync-up nodes
        self.connect_nodes(1, 0)
        self.generate(node0, 1)

        # Request old fork block and expect 'notfound'
        self.send_getdata_and_wait_for_notfound(node0, block_to_request)

        #######################################################################################################
        self.log.info("(c) Test send getdata to a prune node for a block deeper than the NODE_NETWORK_LIMITED threshold")
        self.generate(node1, NODE_NETWORK_LIMITED_MIN_BLOCKS - node1.getblockcount() + 5)
        self.send_getdata_and_wait_for_notfound(node1, node1.getblockhash(height=1))

        #######################################################################################################
        self.log.info("(d) Test send getdata to a prune node for a pruned block")
        self.generate(node1, NODE_NETWORK_LIMITED_MIN_BLOCKS)
        last_pruned_block_height = node1.pruneblockchain(NODE_NETWORK_LIMITED_MIN_BLOCKS)
        assert last_pruned_block_height > 1  # ensure the node pruned the first block
        self.send_getdata_and_wait_for_notfound(node1, node1.getblockhash(height=1))

        #######################################################################################################
        self.log.info("(d) Test getdata for a block that is not on disk")

        # Perturb first block on disk
        with open(node0.chain_path / 'blocks/blk00000.dat', "wb") as bf:
            bf.seek(150)
            bf.write(b"1" * 400)

        self.send_getdata_and_wait_for_notfound(node0, node0.getblockhash(height=1))
        self.send_getdata_and_wait_for_notfound(node0, node0.getblockhash(height=1), block_request_type=MSG_BLOCK | MSG_WITNESS_FLAG)

        #######################################################################################################
        self.log.info("(e) Test getdata (merkleblock) for a peer that does not have TxRelay enabled")
        self.send_getdata_and_wait_for_notfound(node0, node0.getbestblockhash(), block_request_type=MSG_FILTERED_BLOCK)


    def run_test(self):
        self.test_feature_negotiation()
        self.test_getdata_response()


if __name__ == '__main__':
    NotFoundBlockTest().main()
