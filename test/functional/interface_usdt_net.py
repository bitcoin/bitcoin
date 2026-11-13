#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the net:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-net
"""

from io import BytesIO

from test_framework.messages import CBlockHeader, MAX_HEADERS_RESULTS, msg_headers, msg_version
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.usdt import USDT
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)

# from net_address.h
NETWORK_TYPE_UNROUTABLE = 0
# Use in -maxconnections. Results in a maximum of 21 inbound connections
MAX_CONNECTIONS = 53
MAX_INBOUND_CONNECTIONS = 21 # 10 outbound and 1 feeler, (MAX_CONNECTIONS - 10 - 1) / 2 slots for tx-relaying inbounds


class NetTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-maxconnections={MAX_CONNECTIONS}']]

    def start_nodes(self):
        message_fields = [("peer_id", "i"), ("peer_addr", "s"), ("peer_conn_type", "s"),
                          ("msg_type", "s"), ("msg_size", "i"), ("msg", "b")]
        connection_fields = [("id", "i"), ("addr", "s"), ("conn_type", "s"), ("network", "i")]
        self.trace = USDT(self.nodes[0], {
            "net:inbound_message": message_fields,
            "net:outbound_message": message_fields,
            "net:inbound_connection": connection_fields + [("existing", "i")],
            "net:outbound_connection": connection_fields + [("existing", "i")],
            "net:evicted_inbound_connection": connection_fields + [("time_established", "i")],
            "net:closed_connection": connection_fields + [("time_established", "i")],
            "net:misbehaving_connection": [("id", "i"), ("message", "s")],
        })
        super().start_nodes()

    def wait_for_events(self, probes, count):
        events = []

        def received():
            events.extend(event for event in self.trace.read() if event.probe in probes)
            return len(events) >= count

        self.wait_until(received)
        return events

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_usdt_tools()
        self.skip_if_running_under_valgrind()

    def run_test(self):
        for test in (
            self.p2p_message_tracepoint_test,
            self.inbound_conn_tracepoint_test,
            self.outbound_conn_tracepoint_test,
            self.evicted_inbound_conn_tracepoint_test,
            self.misbehaving_conn_tracepoint_test,
            self.closed_conn_tracepoint_test,
        ):
            self.wait_until(lambda: not self.nodes[0].getpeerinfo())
            self.trace.clear()
            test()

    def p2p_message_tracepoint_test(self):
        # Tests the net:inbound_message and net:outbound_message tracepoints
        # See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-net

        EXPECTED_INOUTBOUND_VERSION_MSG = 1
        checked_inbound_version_msg = 0
        checked_outbound_version_msg = 0

        def check_p2p_message(event, is_inbound):
            nonlocal checked_inbound_version_msg, checked_outbound_version_msg
            if event.msg_type == "version":
                self.log.info(
                    f"check_p2p_message(): {'inbound' if is_inbound else 'outbound'} {event}")
                peer = self.nodes[0].getpeerinfo()[0]
                msg = msg_version()
                msg.deserialize(BytesIO(bytes(event.msg[:event.msg_size])))
                assert_equal(peer["id"], event.peer_id, peer["id"])
                assert_equal(peer["addr"], event.peer_addr)
                assert_equal(peer["connection_type"],
                             event.peer_conn_type)
                if is_inbound:
                    checked_inbound_version_msg += 1
                else:
                    checked_outbound_version_msg += 1

        self.log.info("connect a P2P test node to our bitcoind node")
        test_node = P2PInterface()
        self.nodes[0].add_p2p_connection(test_node)
        events = []

        def received_versions():
            events.extend(event for event in self.trace.read()
                          if event.probe in {"net:inbound_message", "net:outbound_message"}
                          and event.msg_type == "version")
            return len(events) >= 2

        self.wait_until(received_versions)

        self.log.info(
            "check receipt and content of in- and outbound version messages")
        for event in events:
            check_p2p_message(event, event.probe == "net:inbound_message")
        assert_equal(EXPECTED_INOUTBOUND_VERSION_MSG,
                     checked_inbound_version_msg)
        assert_equal(EXPECTED_INOUTBOUND_VERSION_MSG,
                     checked_outbound_version_msg)

        test_node.peer_disconnect()

    def inbound_conn_tracepoint_test(self):
        self.log.info("test the net:inbound_connection tracepoint")

        EXPECTED_INBOUND_CONNECTIONS = 2

        self.log.info("connect two P2P test nodes to our bitcoind node")
        testnodes = list()
        for _ in range(EXPECTED_INBOUND_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnodes.append(testnode)
        inbound_connections = self.wait_for_events({"net:inbound_connection"}, EXPECTED_INBOUND_CONNECTIONS)

        assert_equal(EXPECTED_INBOUND_CONNECTIONS, len(inbound_connections))
        for inbound_connection in inbound_connections:
            assert_greater_than(inbound_connection.id, 0)
            assert_greater_than(inbound_connection.existing, 0)
            assert_equal('inbound', inbound_connection.conn_type)
            assert_equal(NETWORK_TYPE_UNROUTABLE, inbound_connection.network)

        for node in testnodes:
            node.peer_disconnect()

    def outbound_conn_tracepoint_test(self):
        self.log.info("test the net:outbound_connection tracepoint")

        EXPECTED_OUTBOUND_CONNECTIONS = 2
        EXPECTED_CONNECTION_TYPE = "feeler"

        self.log.info(
            f"connect {EXPECTED_OUTBOUND_CONNECTIONS} P2P test nodes to our bitcoind node")
        testnodes = list()
        for p2p_idx in range(EXPECTED_OUTBOUND_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_outbound_p2p_connection(
                testnode, p2p_idx=p2p_idx, connection_type=EXPECTED_CONNECTION_TYPE)
            testnodes.append(testnode)
        outbound_connections = self.wait_for_events({"net:outbound_connection"}, EXPECTED_OUTBOUND_CONNECTIONS)

        assert_equal(EXPECTED_OUTBOUND_CONNECTIONS, len(outbound_connections))
        for outbound_connection in outbound_connections:
            assert_greater_than(outbound_connection.id, 0)
            assert_greater_than(outbound_connection.existing, 0)
            assert_equal(EXPECTED_CONNECTION_TYPE, outbound_connection.conn_type)
            assert_equal(NETWORK_TYPE_UNROUTABLE, outbound_connection.network)

        for node in testnodes:
            node.peer_disconnect()

    def evicted_inbound_conn_tracepoint_test(self):
        self.log.info("test the net:evicted_inbound_connection tracepoint")

        EXPECTED_EVICTED_CONNECTIONS = 2

        self.log.info(
            f"connect {MAX_INBOUND_CONNECTIONS + EXPECTED_EVICTED_CONNECTIONS} P2P test nodes to our bitcoind node and expect {EXPECTED_EVICTED_CONNECTIONS} evictions")
        testnodes = list()
        for p2p_idx in range(MAX_INBOUND_CONNECTIONS + EXPECTED_EVICTED_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnodes.append(testnode)
        evicted_connections = self.wait_for_events({"net:evicted_inbound_connection"}, EXPECTED_EVICTED_CONNECTIONS)

        assert_equal(EXPECTED_EVICTED_CONNECTIONS, len(evicted_connections))
        for evicted_connection in evicted_connections:
            assert_greater_than(evicted_connection.id, 0)
            assert_greater_than(evicted_connection.time_established, 0)
            assert_equal("inbound", evicted_connection.conn_type)
            assert_equal(NETWORK_TYPE_UNROUTABLE, evicted_connection.network)

        for node in testnodes:
            node.peer_disconnect()

    def misbehaving_conn_tracepoint_test(self):
        self.log.info("test the net:misbehaving_connection tracepoint")

        EXPECTED_MISBEHAVING_CONNECTIONS = 2
        misbehaving_connections = []

        self.log.info("connect a misbehaving P2P test nodes to our bitcoind node")
        msg = msg_headers([CBlockHeader()] * (MAX_HEADERS_RESULTS + 1))
        for _ in range(EXPECTED_MISBEHAVING_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnode.send_without_ping(msg)
            misbehaving_connections.extend(self.wait_for_events({"net:misbehaving_connection"}, 1))
            testnode.peer_disconnect()

        assert_equal(EXPECTED_MISBEHAVING_CONNECTIONS, len(misbehaving_connections))
        for misbehaving_connection in misbehaving_connections:
            assert_greater_than(misbehaving_connection.id, 0)
            assert_greater_than(len(misbehaving_connection.message), 0)
            assert_equal(misbehaving_connection.message, "headers message size = 2001")

    def closed_conn_tracepoint_test(self):
        self.log.info("test the net:closed_connection tracepoint")

        EXPECTED_CLOSED_CONNECTIONS = 2

        self.log.info(
            f"connect {EXPECTED_CLOSED_CONNECTIONS} P2P test nodes to our bitcoind node")
        testnodes = list()
        for p2p_idx in range(EXPECTED_CLOSED_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnodes.append(testnode)
        for node in testnodes:
            node.peer_disconnect()
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 0)
        closed_connections = self.wait_for_events({"net:closed_connection"}, EXPECTED_CLOSED_CONNECTIONS)

        assert_equal(EXPECTED_CLOSED_CONNECTIONS, len(closed_connections))
        for closed_connection in closed_connections:
            assert_greater_than(closed_connection.id, 0)
            assert_equal("inbound", closed_connection.conn_type)
            assert_equal(0, closed_connection.network)
            assert_greater_than(closed_connection.time_established, 0)


if __name__ == '__main__':
    NetTracepointTest(__file__).main()
