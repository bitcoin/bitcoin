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
    from bcc import BPF, USDT # type: ignore[import]
except ImportError:
    pass
from test_framework.messages import msg_version
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

# Tor v3 addresses are 62 chars + 6 chars for the port (':12345').
MAX_PEER_ADDR_LENGTH = 68
MAX_PEER_CONN_TYPE_LENGTH = 20
MAX_MSG_TYPE_LENGTH = 20
# We won't process messages larger than 150 byte in this test. For reading
# larger messanges see contrib/tracing/log_raw_p2p_msgs.py
MAX_MSG_DATA_LENGTH = 150

net_tracepoints_program = """
#include <uapi/linux/ptrace.h>

#define MAX_PEER_ADDR_LENGTH {}
#define MAX_PEER_CONN_TYPE_LENGTH {}
#define MAX_MSG_TYPE_LENGTH {}
#define MAX_MSG_DATA_LENGTH {}
""".format(
    MAX_PEER_ADDR_LENGTH,
    MAX_PEER_CONN_TYPE_LENGTH,
    MAX_MSG_TYPE_LENGTH,
    MAX_MSG_DATA_LENGTH
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
"""


class NetTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_python_bcc()
        self.skip_if_no_bpf_permissions()

    def run_test(self):
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


if __name__ == '__main__':
    NetTracepointTest(__file__).main()
