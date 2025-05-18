#!/usr/bin/env python3
# Copyright (c) 2021-present The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test v2 transport
"""
import socket

from test_framework.messages import MAGIC_BYTES, NODE_P2P_V2
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
    assert_raises_rpc_error
)


class V2TransportTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.extra_args = [["-v2transport=1"], ["-v2transport=1"], ["-v2transport=0"], ["-v2transport=0"], ["-v2transport=0"]]

    def run_test(self):
        sending_handshake = "start sending v2 handshake to peer"
        downgrading_to_v1 = "retrying with v1 transport protocol for peer"
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(1, 2)
        self.disconnect_nodes(2, 3)
        self.disconnect_nodes(3, 4)

        # verify local services
        network_info = self.nodes[2].getnetworkinfo()
        assert_equal(int(network_info["localservices"], 16) & NODE_P2P_V2, 0)
        assert "P2P_V2" not in network_info["localservicesnames"]
        network_info = self.nodes[1].getnetworkinfo()
        assert_equal(int(network_info["localservices"], 16) & NODE_P2P_V2, NODE_P2P_V2)
        assert "P2P_V2" in network_info["localservicesnames"]

        # V2 nodes can sync with V2 nodes
        assert_equal(self.nodes[0].getblockcount(), 0)
        assert_equal(self.nodes[1].getblockcount(), 0)
        with self.nodes[0].assert_debug_log(expected_msgs=[sending_handshake],
                                            unexpected_msgs=[downgrading_to_v1]):
            self.connect_nodes(0, 1, peer_advertises_v2=True)
        self.generate(self.nodes[0], 5, sync_fun=lambda: self.sync_all(self.nodes[0:2]))
        assert_equal(self.nodes[1].getblockcount(), 5)
        # verify there is a v2 connection between node 0 and 1
        node_0_info = self.nodes[0].getpeerinfo()
        node_1_info = self.nodes[1].getpeerinfo()
        assert_equal(len(node_0_info), 1)
        assert_equal(len(node_1_info), 1)
        assert_equal(node_0_info[0]["transport_protocol_type"], "v2")
        assert_equal(node_1_info[0]["transport_protocol_type"], "v2")
        assert_equal(len(node_0_info[0]["session_id"]), 64)
        assert_equal(len(node_1_info[0]["session_id"]), 64)
        assert_equal(node_0_info[0]["session_id"], node_1_info[0]["session_id"])

        # V1 nodes can sync with each other
        assert_equal(self.nodes[2].getblockcount(), 0)
        assert_equal(self.nodes[3].getblockcount(), 0)

        # addnode rpc error when v2transport requested but not enabled
        ip_port = "127.0.0.1:{}".format(p2p_port(3))
        assert_raises_rpc_error(-8, "Error: v2transport requested but not enabled (see -v2transport)", self.nodes[2].addnode, node=ip_port, command='add', v2transport=True)

        with self.nodes[2].assert_debug_log(expected_msgs=[],
                                            unexpected_msgs=[sending_handshake, downgrading_to_v1]):
            self.connect_nodes(2, 3, peer_advertises_v2=False)
        self.generate(self.nodes[2], 8, sync_fun=lambda: self.sync_all(self.nodes[2:4]))
        assert_equal(self.nodes[3].getblockcount(), 8)
        assert self.nodes[0].getbestblockhash() != self.nodes[2].getbestblockhash()
        # verify there is a v1 connection between node 2 and 3
        node_2_info = self.nodes[2].getpeerinfo()
        node_3_info = self.nodes[3].getpeerinfo()
        assert_equal(len(node_2_info), 1)
        assert_equal(len(node_3_info), 1)
        assert_equal(node_2_info[0]["transport_protocol_type"], "v1")
        assert_equal(node_3_info[0]["transport_protocol_type"], "v1")
        assert_equal(len(node_2_info[0]["session_id"]), 0)
        assert_equal(len(node_3_info[0]["session_id"]), 0)

        # V1 nodes can sync with V2 nodes
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(2, 3)
        with self.nodes[2].assert_debug_log(expected_msgs=[],
                                            unexpected_msgs=[sending_handshake, downgrading_to_v1]):
            self.connect_nodes(2, 1, peer_advertises_v2=False) # cannot enable v2 on v1 node
        self.sync_all(self.nodes[1:3])
        assert_equal(self.nodes[1].getblockcount(), 8)
        assert self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash()
        # verify there is a v1 connection between node 1 and 2
        node_1_info = self.nodes[1].getpeerinfo()
        node_2_info = self.nodes[2].getpeerinfo()
        assert_equal(len(node_1_info), 1)
        assert_equal(len(node_2_info), 1)
        assert_equal(node_1_info[0]["transport_protocol_type"], "v1")
        assert_equal(node_2_info[0]["transport_protocol_type"], "v1")
        assert_equal(len(node_1_info[0]["session_id"]), 0)
        assert_equal(len(node_2_info[0]["session_id"]), 0)

        # V2 nodes can sync with V1 nodes
        self.disconnect_nodes(1, 2)
        with self.nodes[0].assert_debug_log(expected_msgs=[],
                                            unexpected_msgs=[sending_handshake, downgrading_to_v1]):
            self.connect_nodes(0, 3, peer_advertises_v2=False)
        self.sync_all([self.nodes[0], self.nodes[3]])
        assert_equal(self.nodes[0].getblockcount(), 8)
        # verify there is a v1 connection between node 0 and 3
        node_0_info = self.nodes[0].getpeerinfo()
        node_3_info = self.nodes[3].getpeerinfo()
        assert_equal(len(node_0_info), 1)
        assert_equal(len(node_3_info), 1)
        assert_equal(node_0_info[0]["transport_protocol_type"], "v1")
        assert_equal(node_3_info[0]["transport_protocol_type"], "v1")
        assert_equal(len(node_0_info[0]["session_id"]), 0)
        assert_equal(len(node_3_info[0]["session_id"]), 0)

        # V2 node mines another block and everyone gets it
        self.connect_nodes(0, 1, peer_advertises_v2=True)
        self.connect_nodes(1, 2, peer_advertises_v2=False)
        self.generate(self.nodes[1], 1, sync_fun=lambda: self.sync_all(self.nodes[0:4]))
        assert_equal(self.nodes[0].getblockcount(), 9) # sync_all() verifies tip hashes match

        # V1 node mines another block and everyone gets it
        self.generate(self.nodes[3], 2, sync_fun=lambda: self.sync_all(self.nodes[0:4]))
        assert_equal(self.nodes[2].getblockcount(), 11) # sync_all() verifies tip hashes match

        assert_equal(self.nodes[4].getblockcount(), 0)
        # Peer 4 is v1 p2p, but is falsely advertised as v2.
        with self.nodes[1].assert_debug_log(expected_msgs=[sending_handshake, downgrading_to_v1]):
            self.connect_nodes(1, 4, peer_advertises_v2=True)
        self.sync_all()
        assert_equal(self.nodes[4].getblockcount(), 11)

        # Check v1 prefix detection
        V1_PREFIX = MAGIC_BYTES["regtest"] + b"version\x00\x00\x00\x00\x00"
        assert_equal(len(V1_PREFIX), 16)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            with self.nodes[0].wait_for_new_peer():
                s.connect(("127.0.0.1", p2p_port(0)))
            s.sendall(V1_PREFIX[:-1])
            assert_equal(self.nodes[0].getpeerinfo()[-1]["transport_protocol_type"], "detecting")
            s.sendall(bytes([V1_PREFIX[-1]]))  # send out last prefix byte
            self.wait_until(lambda: self.nodes[0].getpeerinfo()[-1]["transport_protocol_type"] == "v1")

        # Check wrong network prefix detection (hits if the next 12 bytes correspond to a v1 version message)
        wrong_network_magic_prefix = MAGIC_BYTES["signet"] + V1_PREFIX[4:]
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            with self.nodes[0].wait_for_new_peer():
                s.connect(("127.0.0.1", p2p_port(0)))
            with self.nodes[0].assert_debug_log(["V2 transport error: V1 peer with wrong MessageStart"]):
                s.sendall(wrong_network_magic_prefix + b"somepayload")

        # Check detection of missing garbage terminator (hits after fixed amount of data if terminator never matches garbage)
        MAX_KEY_GARB_AND_GARBTERM_LEN = 64 + 4095 + 16
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            with self.nodes[0].wait_for_new_peer():
                s.connect(("127.0.0.1", p2p_port(0)))
            s.sendall(b'\x00' * (MAX_KEY_GARB_AND_GARBTERM_LEN - 1))
            self.wait_until(lambda: self.nodes[0].getpeerinfo()[-1]["bytesrecv"] == MAX_KEY_GARB_AND_GARBTERM_LEN - 1)
            with self.nodes[0].assert_debug_log(["V2 transport error: missing garbage terminator"]):
                peer_id = self.nodes[0].getpeerinfo()[-1]["id"]
                s.sendall(b'\x00')  # send out last byte
                # should disconnect immediately
                self.wait_until(lambda: not peer_id in [p["id"] for p in self.nodes[0].getpeerinfo()])


if __name__ == '__main__':
    V2TransportTest(__file__).main()
