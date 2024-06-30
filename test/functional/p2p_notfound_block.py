#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test notfound block responses
"""

from collections import defaultdict

from test_framework.messages import (
    CBlockHeader,
    CInv,
    from_hex,
    msg_getdata,
    msg_headers,
    msg_notfound,
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


class GetDataTrackerConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.vec_getdata = []

    def on_getdata(self, message):
        self.vec_getdata += message.inv


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

    def test_notfound_handler(self):
        self.log.info("Test 'notfound' msg handler")
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        self.log.info("(a) Send not_found for a block the peer did not request")

        conn = node0.add_p2p_connection(P2PInterface())
        with node0.assert_debug_log(["Misbehaving: peer=11: sent 'notfound' for a block we don't requested"]):
            conn.send_message(msg_notfound([CInv(t=MSG_BLOCK, h=0)]))
            conn.wait_for_disconnect()

        ################################################################################
        self.log.info("(b) Send not_found for a block the peer requested but not to us")

        # Generate block, in isolation, then relay header from a connection handled by us
        self.sync_blocks()
        self.disconnect_nodes(0, 1)
        self.generate(node1, 1, sync_fun=self.no_op)
        tip_header = from_hex(CBlockHeader(), node1.getblockheader(node1.getbestblockhash(), verbose=False))
        tip_header.calc_sha256()

        # Send tip header and wait for getdata request without responding to it.
        conn = node0.add_p2p_connection(P2PInterface())
        conn.send_message(msg_headers([tip_header]))
        conn.wait_for_getdata([tip_header.sha256])

        # Now send 'notfound' from another connection and expect disconnection.
        conn = node0.add_p2p_connection(P2PInterface())
        with node0.assert_debug_log(["Misbehaving: peer=13: sent 'notfound' for a block we don't requested"]):
            conn.send_message(msg_notfound([CInv(t=MSG_BLOCK, h=tip_header.sha256)]))
            conn.wait_for_disconnect()

        ################################################################################
        self.log.info("(c) Send not_found for an historical block")

        # Context: node1 is already pruned.
        # Restart it with the -reindex flag enabled so it resync blocks.
        node_time = node0.getblockheader(node0.getbestblockhash())['time']  # set mocktime so it can be advanced later
        self.restart_node(1, extra_args=["-reindex", f'-mocktime={node_time}'])
        conn = node1.add_p2p_connection(GetDataTrackerConn())
        conn.wait_for_getheaders()

        # Send header and wait for getdata
        block_1_header = from_hex(CBlockHeader(), node0.getblockheader(node0.getblockhash(1), verbose=False))
        conn.send_message(msg_headers([block_1_header]))
        self.wait_until(lambda: len(conn.vec_getdata) > 0)

        # Now we received the getdata, send not_found
        inv = conn.vec_getdata[0]
        conn.send_message(msg_notfound([inv]))
        # Check the block was properly marked as missing
        block_to_request = inv.hash.to_bytes(32, 'big').hex()
        self.wait_until(lambda: block_to_request in node1.getpeerinfo()[-1]['missing_blocks'])

        # Verify the node will retry to download the block after 24h (when the 'missing block' entry expires)
        node_time += 60 * 60 * 24 + 1
        node1.setmocktime(node_time)
        self.wait_until(lambda: block_to_request not in node1.getpeerinfo()[-1]['missing_blocks'])
        assert_equal(len(conn.vec_getdata), 2)  # two requests for the same block

        ################################################################################
        self.log.info("(d) Send multiple not_found to surpass the accepted limit")
        MAX_MISSING_BLOCKS_NUM = 30

        msg = msg_headers()
        for block_num in range(2, MAX_MISSING_BLOCKS_NUM + 3):  # range of 31 blocks
            msg.headers.append(from_hex(CBlockHeader(), node0.getblockheader(node0.getblockhash(block_num), verbose=False)))
        conn.vec_getdata.clear()  # remove all previous requests
        conn.send_message(msg)

        # As blocks are requested in windows of 15 blocks max, once we respond with the not_found, the node will try
        # to download blocks that follows the last not_found block, and we will continue responding with a not_found
        # until the limit is reached.
        num_notfound_sent = 0
        with node1.assert_debug_log(["peer=0 exceeded max missing blocks number, disconnecting.."]):
            while num_notfound_sent <= 30:
                self.wait_until(lambda: len(conn.vec_getdata) > 0)
                not_found_msg = msg_notfound(conn.vec_getdata.copy())
                conn.vec_getdata.clear()  # remove all previous requests
                conn.send_message(not_found_msg)
                num_notfound_sent += len(not_found_msg.vec)
            conn.wait_for_disconnect()

        # TODO: Test: when the node is synced and receives a notfound for a block close to the tip
        # Now connect the real node, so it can sync-up the entire chain
        #self.connect_nodes(1, 0)
        #self.sync_blocks()

    def run_test(self):
        self.test_feature_negotiation()
        self.test_getdata_response()
        self.test_notfound_handler()


if __name__ == '__main__':
    NotFoundBlockTest().main()
