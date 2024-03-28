#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the net:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-net
"""

import ctypes
from io import BytesIO
# Test will be skipped if we don't have bcc installed
try:
    from bcc import BPF, USDT  # type: ignore[import]
except ImportError:
    pass
from test_framework.messages import CBlockHeader, MAX_HEADERS_RESULTS, msg_headers, msg_version
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

# Tor v3 addresses are 62 chars + 6 chars for the port (':12345').
MAX_PEER_ADDR_LENGTH = 68
MAX_PEER_CONN_TYPE_LENGTH = 20
MAX_MSG_TYPE_LENGTH = 20
MAX_MISBEHAVING_MESSAGE_LENGTH = 128
# We won't process messages larger than 150 byte in this test. For reading
# larger messanges see contrib/tracing/log_raw_p2p_msgs.py
MAX_MSG_DATA_LENGTH = 150

# from net_address.h
NETWORK_TYPE_UNROUTABLE = 0
# Use in -maxconnections. Results in a maximum of 21 inbound connections
MAX_CONNECTIONS = 32
MAX_INBOUND_CONNECTIONS = MAX_CONNECTIONS - 10 - 1  # 10 outbound and 1 feeler

# from src/net_processing.h
MISBEHAVING_DISCOURAGEMENT_THRESHOLD = 100

net_tracepoints_program = """
#include <uapi/linux/ptrace.h>

#define MAX_PEER_ADDR_LENGTH {}
#define MAX_PEER_CONN_TYPE_LENGTH {}
#define MAX_MSG_TYPE_LENGTH {}
#define MAX_MSG_DATA_LENGTH {}
#define MAX_MISBEHAVING_MESSAGE_LENGTH {}
""".format(
    MAX_PEER_ADDR_LENGTH,
    MAX_PEER_CONN_TYPE_LENGTH,
    MAX_MSG_TYPE_LENGTH,
    MAX_MSG_DATA_LENGTH,
    MAX_MISBEHAVING_MESSAGE_LENGTH
) + """
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

struct p2p_message
{
    u64     peer_id;
    char    peer_addr[MAX_PEER_ADDR_LENGTH];
    char    peer_conn_type[MAX_PEER_CONN_TYPE_LENGTH];
    char    msg_type[MAX_MSG_TYPE_LENGTH];
    u64     msg_size;
    u8      msg[MAX_MSG_DATA_LENGTH];
};

struct Connection
{
    u64     id;
    char    addr[MAX_PEER_ADDR_LENGTH];
    char    type[MAX_PEER_CONN_TYPE_LENGTH];
    u32     network;
};

struct NewConnection
{
    struct Connection   conn;
    u64                 existing;
};

struct ClosedConnection
{
    struct Connection   conn;
    u64                 time_established;
};

struct MisbehavingConnection
{
    u64     id;
    s32     score_before;
    s32     howmuch;
    char    message[MAX_MISBEHAVING_MESSAGE_LENGTH];
    bool    threshold_exceeded;
};

BPF_PERF_OUTPUT(inbound_messages);
int trace_inbound_message(struct pt_regs *ctx) {
    struct p2p_message msg = {};
    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg_p(2, ctx, &msg.peer_addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &msg.peer_conn_type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg_p(4, ctx, &msg.msg_type, MAX_MSG_TYPE_LENGTH);
    bpf_usdt_readarg(5, ctx, &msg.msg_size);
    bpf_usdt_readarg_p(6, ctx, &msg.msg, MIN(msg.msg_size, MAX_MSG_DATA_LENGTH));
    inbound_messages.perf_submit(ctx, &msg, sizeof(msg));
    return 0;
}

BPF_PERF_OUTPUT(outbound_messages);
int trace_outbound_message(struct pt_regs *ctx) {
    struct p2p_message msg = {};
    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg_p(2, ctx, &msg.peer_addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &msg.peer_conn_type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg_p(4, ctx, &msg.msg_type, MAX_MSG_TYPE_LENGTH);
    bpf_usdt_readarg(5, ctx, &msg.msg_size);
    bpf_usdt_readarg_p(6, ctx, &msg.msg, MIN(msg.msg_size, MAX_MSG_DATA_LENGTH));
    outbound_messages.perf_submit(ctx, &msg, sizeof(msg));
    return 0;
};

BPF_PERF_OUTPUT(inbound_connections);
int trace_inbound_connection(struct pt_regs *ctx) {
    struct NewConnection inbound = {};
    bpf_usdt_readarg(1, ctx, &inbound.conn.id);
    bpf_usdt_readarg_p(2, ctx, &inbound.conn.addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &inbound.conn.type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg(4, ctx, &inbound.conn.network);
    bpf_usdt_readarg(5, ctx, &inbound.existing);
    inbound_connections.perf_submit(ctx, &inbound, sizeof(inbound));
    return 0;
};

BPF_PERF_OUTPUT(outbound_connections);
int trace_outbound_connection(struct pt_regs *ctx) {
    struct NewConnection outbound = {};
    bpf_usdt_readarg(1, ctx, &outbound.conn.id);
    bpf_usdt_readarg_p(2, ctx, &outbound.conn.addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &outbound.conn.type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg(4, ctx, &outbound.conn.network);
    bpf_usdt_readarg(5, ctx, &outbound.existing);
    outbound_connections.perf_submit(ctx, &outbound, sizeof(outbound));
    return 0;
};

BPF_PERF_OUTPUT(evicted_inbound_connections);
int trace_evicted_inbound_connection(struct pt_regs *ctx) {
    struct ClosedConnection evicted = {};
    bpf_usdt_readarg(1, ctx, &evicted.conn.id);
    bpf_usdt_readarg_p(2, ctx, &evicted.conn.addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &evicted.conn.type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg(4, ctx, &evicted.conn.network);
    bpf_usdt_readarg(5, ctx, &evicted.time_established);
    evicted_inbound_connections.perf_submit(ctx, &evicted, sizeof(evicted));
    return 0;
};

BPF_PERF_OUTPUT(misbehaving_connections);
int trace_misbehaving_connection(struct pt_regs *ctx) {
    struct MisbehavingConnection misbehaving = {};
    bpf_usdt_readarg(1, ctx, &misbehaving.id);
    bpf_usdt_readarg(2, ctx, &misbehaving.score_before);
    bpf_usdt_readarg(3, ctx, &misbehaving.howmuch);
    bpf_usdt_readarg_p(4, ctx, &misbehaving.message, 64);
    bpf_usdt_readarg(5, ctx, &misbehaving.threshold_exceeded);
    misbehaving_connections.perf_submit(ctx, &misbehaving, sizeof(misbehaving));
    return 0;
};

BPF_PERF_OUTPUT(closed_connections);
int trace_closed_connection(struct pt_regs *ctx) {
    struct ClosedConnection closed = {};
    bpf_usdt_readarg(1, ctx, &closed.conn.id);
    bpf_usdt_readarg_p(2, ctx, &closed.conn.addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &closed.conn.type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg(4, ctx, &closed.conn.network);
    bpf_usdt_readarg(5, ctx, &closed.time_established);
    closed_connections.perf_submit(ctx, &closed, sizeof(closed));
    return 0;
};
"""


class Connection(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint64),
        ("addr", ctypes.c_char * MAX_PEER_ADDR_LENGTH),
        ("conn_type", ctypes.c_char * MAX_PEER_CONN_TYPE_LENGTH),
        ("network", ctypes.c_uint32),
    ]

    def __repr__(self):
        return f"Connection(peer={self.id}, addr={self.addr.decode('utf-8')}, conn_type={self.conn_type.decode('utf-8')}, network={self.network})"


class NewConnection(ctypes.Structure):
    _fields_ = [
        ("conn", Connection),
        ("existing", ctypes.c_uint64),
    ]

    def __repr__(self):
        return f"NewConnection(conn={self.conn}, existing={self.existing})"


class ClosedConnection(ctypes.Structure):
    _fields_ = [
        ("conn", Connection),
        ("time_established", ctypes.c_uint64),
    ]

    def __repr__(self):
        return f"ClosedConnection(conn={self.conn}, time_established={self.time_established})"


class MisbehavingConnection(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint64),
        ("score_before", ctypes.c_int32),
        ("howmuch", ctypes.c_int32),
        ("message", ctypes.c_char * MAX_MISBEHAVING_MESSAGE_LENGTH),
        ("threshold_exceeded", ctypes.c_bool),
    ]

    def __repr__(self):
        return f"MisbehavingConnection(id={self.id}, score_before={self.score_before}, howmuch={self.howmuch}, message={self.message}, threshold_exceeded={self.threshold_exceeded})"


class NetTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-maxconnections={MAX_CONNECTIONS}']]

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_python_bcc()
        self.skip_if_no_bpf_permissions()

    def run_test(self):
        self.p2p_message_tracepoint_test()
        self.inbound_conn_tracepoint_test()
        self.outbound_conn_tracepoint_test()
        self.evicted_inbound_conn_tracepoint_test()
        self.misbehaving_conn_tracepoint_test()
        self.closed_conn_tracepoint_test()

    def p2p_message_tracepoint_test(self):
        # Tests the net:inbound_message and net:outbound_message tracepoints
        # See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-net

        class P2PMessage(ctypes.Structure):
            _fields_ = [
                ("peer_id", ctypes.c_uint64),
                ("peer_addr", ctypes.c_char * MAX_PEER_ADDR_LENGTH),
                ("peer_conn_type", ctypes.c_char * MAX_PEER_CONN_TYPE_LENGTH),
                ("msg_type", ctypes.c_char * MAX_MSG_TYPE_LENGTH),
                ("msg_size", ctypes.c_uint64),
                ("msg", ctypes.c_ubyte * MAX_MSG_DATA_LENGTH),
            ]

            def __repr__(self):
                return f"P2PMessage(peer={self.peer_id}, addr={self.peer_addr.decode('utf-8')}, conn_type={self.peer_conn_type.decode('utf-8')}, msg_type={self.msg_type.decode('utf-8')}, msg_size={self.msg_size})"

        self.log.info(
            "hook into the net:inbound_message and net:outbound_message tracepoints")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:inbound_message",
                         fn_name="trace_inbound_message")
        ctx.enable_probe(probe="net:outbound_message",
                         fn_name="trace_outbound_message")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        EXPECTED_INOUTBOUND_VERSION_MSG = 1
        checked_inbound_version_msg = 0
        checked_outbound_version_msg = 0
        events = []

        def check_p2p_message(event, is_inbound):
            nonlocal checked_inbound_version_msg, checked_outbound_version_msg
            if event.msg_type.decode("utf-8") == "version":
                self.log.info(
                    f"check_p2p_message(): {'inbound' if is_inbound else 'outbound'} {event}")
                peer = self.nodes[0].getpeerinfo()[0]
                msg = msg_version()
                msg.deserialize(BytesIO(bytes(event.msg[:event.msg_size])))
                assert_equal(peer["id"], event.peer_id, peer["id"])
                assert_equal(peer["addr"], event.peer_addr.decode("utf-8"))
                assert_equal(peer["connection_type"],
                             event.peer_conn_type.decode("utf-8"))
                if is_inbound:
                    checked_inbound_version_msg += 1
                else:
                    checked_outbound_version_msg += 1

        def handle_inbound(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(P2PMessage)).contents
            events.append((event, True))

        def handle_outbound(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(P2PMessage)).contents
            events.append((event, False))

        bpf["inbound_messages"].open_perf_buffer(handle_inbound)
        bpf["outbound_messages"].open_perf_buffer(handle_outbound)

        self.log.info("connect a P2P test node to our bitcoind node")
        test_node = P2PInterface()
        self.nodes[0].add_p2p_connection(test_node)
        bpf.perf_buffer_poll(timeout=200)

        self.log.info(
            "check receipt and content of in- and outbound version messages")
        for event, is_inbound in events:
            check_p2p_message(event, is_inbound)
        assert_equal(EXPECTED_INOUTBOUND_VERSION_MSG,
                     checked_inbound_version_msg)
        assert_equal(EXPECTED_INOUTBOUND_VERSION_MSG,
                     checked_outbound_version_msg)


        bpf.cleanup()
        test_node.peer_disconnect()

    def inbound_conn_tracepoint_test(self):
        self.log.info("hook into the net:inbound_connection tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:inbound_connection",
                         fn_name="trace_inbound_connection")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        inbound_connections = []
        EXPECTED_INBOUND_CONNECTIONS = 2

        def handle_inbound_connection(_, data, __):
            nonlocal inbound_connections
            event = ctypes.cast(data, ctypes.POINTER(NewConnection)).contents
            self.log.info(f"handle_inbound_connection(): {event}")
            inbound_connections.append(event)

        bpf["inbound_connections"].open_perf_buffer(handle_inbound_connection)

        self.log.info("connect two P2P test nodes to our bitcoind node")
        testnodes = list()
        for _ in range(EXPECTED_INBOUND_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnodes.append(testnode)
        bpf.perf_buffer_poll(timeout=200)

        assert_equal(EXPECTED_INBOUND_CONNECTIONS, len(inbound_connections))
        for inbound_connection in inbound_connections:
            assert inbound_connection.conn.id > 0
            assert inbound_connection.existing >= 0
            assert_equal(b'inbound', inbound_connection.conn.conn_type)
            assert_equal(NETWORK_TYPE_UNROUTABLE, inbound_connection.conn.network)

        bpf.cleanup()
        for node in testnodes:
            node.peer_disconnect()

    def outbound_conn_tracepoint_test(self):
        self.log.info("hook into the net:outbound_connection tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:outbound_connection",
                         fn_name="trace_outbound_connection")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        # that the handle_* function succeeds.
        EXPECTED_OUTBOUND_CONNECTIONS = 2
        EXPECTED_CONNECTION_TYPE = "feeler"
        outbound_connections = []

        def handle_outbound_connection(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(NewConnection)).contents
            self.log.info(f"handle_outbound_connection(): {event}")
            outbound_connections.append(event)

        bpf["outbound_connections"].open_perf_buffer(
            handle_outbound_connection)

        self.log.info(
            f"connect {EXPECTED_OUTBOUND_CONNECTIONS} P2P test nodes to our bitcoind node")
        testnodes = list()
        for p2p_idx in range(EXPECTED_OUTBOUND_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_outbound_p2p_connection(
                testnode, p2p_idx=p2p_idx, connection_type=EXPECTED_CONNECTION_TYPE)
            testnodes.append(testnode)
        bpf.perf_buffer_poll(timeout=200)

        assert_equal(EXPECTED_OUTBOUND_CONNECTIONS, len(outbound_connections))
        for outbound_connection in outbound_connections:
            assert outbound_connection.conn.id > 0
            assert outbound_connection.existing >= 0
            assert_equal(EXPECTED_CONNECTION_TYPE, outbound_connection.conn.conn_type.decode('utf-8'))
            assert_equal(NETWORK_TYPE_UNROUTABLE, outbound_connection.conn.network)

        bpf.cleanup()
        for node in testnodes:
            node.peer_disconnect()

    def evicted_inbound_conn_tracepoint_test(self):
        self.log.info("hook into the net:evicted_inbound_connection tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:evicted_inbound_connection",
                         fn_name="trace_evicted_inbound_connection")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        EXPECTED_EVICTED_CONNECTIONS = 2
        evicted_connections = []

        def handle_evicted_inbound_connection(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(ClosedConnection)).contents
            self.log.info(f"handle_evicted_inbound_connection(): {event}")
            evicted_connections.append(event)

        bpf["evicted_inbound_connections"].open_perf_buffer(handle_evicted_inbound_connection)

        self.log.info(
            f"connect {MAX_INBOUND_CONNECTIONS + EXPECTED_EVICTED_CONNECTIONS} P2P test nodes to our bitcoind node and expect {EXPECTED_EVICTED_CONNECTIONS} evictions")
        testnodes = list()
        for p2p_idx in range(MAX_INBOUND_CONNECTIONS + EXPECTED_EVICTED_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnodes.append(testnode)
        bpf.perf_buffer_poll(timeout=200)

        assert_equal(EXPECTED_EVICTED_CONNECTIONS, len(evicted_connections))
        for evicted_connection in evicted_connections:
            assert evicted_connection.conn.id > 0
            assert evicted_connection.time_established > 0
            assert_equal("inbound", evicted_connection.conn.conn_type.decode('utf-8'))
            assert_equal(NETWORK_TYPE_UNROUTABLE, evicted_connection.conn.network)

        bpf.cleanup()
        for node in testnodes:
            node.peer_disconnect()

    def misbehaving_conn_tracepoint_test(self):
        self.log.info("hook into the net:misbehaving_connection tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:misbehaving_connection",
                         fn_name="trace_misbehaving_connection")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        EXPECTED_MISBEHAVING_CONNECTIONS = 5
        misbehaving_connections = []

        def handle_misbehaving_connection(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(MisbehavingConnection)).contents
            self.log.info(f"handle_misbehaving_connection(): {event}")
            misbehaving_connections.append(event)

        bpf["misbehaving_connections"].open_perf_buffer(handle_misbehaving_connection)

        self.log.info(
            f"connect a misbehaving P2P test nodes to our bitcoind node")
        testnode = P2PInterface()
        self.nodes[0].add_p2p_connection(testnode)
        msg = msg_headers([CBlockHeader()] * (MAX_HEADERS_RESULTS + 1))
        for _ in range(EXPECTED_MISBEHAVING_CONNECTIONS):
            testnode.send_message(msg)
            bpf.perf_buffer_poll(timeout=500)

        assert_equal(EXPECTED_MISBEHAVING_CONNECTIONS, len(misbehaving_connections))
        misbehaving_score = 0
        for misbehaving_connection in misbehaving_connections:
            assert misbehaving_connection.id > 0
            assert misbehaving_connection.howmuch > 0
            assert len(misbehaving_connection.message) > 0
            assert_equal(misbehaving_score, misbehaving_connection.score_before)
            misbehaving_score += misbehaving_connection.howmuch
            assert_equal(misbehaving_score >= MISBEHAVING_DISCOURAGEMENT_THRESHOLD, misbehaving_connection.threshold_exceeded)

        bpf.cleanup()
        testnode.peer_disconnect()

    def closed_conn_tracepoint_test(self):
        self.log.info("hook into the net:closed_connection tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:closed_connection",
                         fn_name="trace_closed_connection")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        EXPECTED_CLOSED_CONNECTIONS = 2
        closed_connections = []

        def handle_closed_connection(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(ClosedConnection)).contents
            self.log.info(f"handle_closed_connection(): {event}")
            closed_connections.append(event)

        bpf["closed_connections"].open_perf_buffer(handle_closed_connection)

        self.log.info(
            f"connect {EXPECTED_CLOSED_CONNECTIONS} P2P test nodes to our bitcoind node")
        testnodes = list()
        for p2p_idx in range(EXPECTED_CLOSED_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnodes.append(testnode)
        for node in testnodes:
            node.peer_disconnect()
        bpf.perf_buffer_poll(timeout=400)

        assert_equal(EXPECTED_CLOSED_CONNECTIONS, len(closed_connections))
        for closed_connection in closed_connections:
            assert closed_connection.conn.id > 0
            assert_equal("inbound", closed_connection.conn.conn_type.decode('utf-8'))
            assert_equal(0, closed_connection.conn.network)
            assert closed_connection.time_established > 0

        bpf.cleanup()

if __name__ == '__main__':
    NetTracepointTest().main()
