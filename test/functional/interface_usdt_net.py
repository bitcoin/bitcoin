#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the net:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-net
"""

import ctypes
from decimal import Decimal
from io import BytesIO
# Test will be skipped if we don't have bcc installed
try:
    from bcc import BPF, USDT  # type: ignore[import]
except ImportError:
    pass
from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    add_witness_commitment,
    create_block,
)
from test_framework.messages import (
    CBlockHeader,
    HeaderAndShortIDs,
    MAX_BIP125_RBF_SEQUENCE,
    MAX_HEADERS_RESULTS,
    msg_cmpctblock,
    msg_headers,
    msg_tx,
    msg_version,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    bpf_cflags,
)
from test_framework.wallet import MiniWallet

# Tor v3 addresses are 62 chars + 6 chars for the port (':12345').
MAX_PEER_ADDR_LENGTH = 68
MAX_PEER_CONN_TYPE_LENGTH = 20
MAX_MSG_TYPE_LENGTH = 20
MAX_MISBEHAVING_MESSAGE_LENGTH = 128
BLOCK_HASH_LENGTH = 32
# We won't process messages larger than 150 byte in this test. For reading
# larger messages see contrib/tracing/log_raw_p2p_msgs.py
MAX_MSG_DATA_LENGTH = 150

# from net_address.h
NETWORK_TYPE_UNROUTABLE = 0
# Use in -maxconnections. Results in a maximum of 21 inbound connections
MAX_CONNECTIONS = 32
MAX_INBOUND_CONNECTIONS = MAX_CONNECTIONS - 10 - 1  # 10 outbound and 1 feeler

net_tracepoints_program = """
#include <uapi/linux/ptrace.h>

#define MAX_PEER_ADDR_LENGTH {}
#define MAX_PEER_CONN_TYPE_LENGTH {}
#define MAX_MSG_TYPE_LENGTH {}
#define MAX_MSG_DATA_LENGTH {}
#define MAX_MISBEHAVING_MESSAGE_LENGTH {}
#define BLOCK_HASH_LENGTH {}
""".format(
    MAX_PEER_ADDR_LENGTH,
    MAX_PEER_CONN_TYPE_LENGTH,
    MAX_MSG_TYPE_LENGTH,
    MAX_MSG_DATA_LENGTH,
    MAX_MISBEHAVING_MESSAGE_LENGTH,
    BLOCK_HASH_LENGTH,
) + """
// A min() macro. Prefixed with _TRACEPOINT_TEST to avoid collision with other MIN macros.
#define _TRACEPOINT_TEST_MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

typedef signed long long i64;

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
    char    message[MAX_MISBEHAVING_MESSAGE_LENGTH];
};

struct BlockHeader
{
    int     height;
    i64     peer_id;
    bool    via_compact_block;
    u8      hash[BLOCK_HASH_LENGTH];
};

struct CompactBlockReconstructed
{
    i64     peer_id;
    u32     prefilled_txn_count;
    u32     mempool_txn_count;
    u32     extra_pool_txn_count;
    u32     requested_txn_count;
    u32     requested_txn_bytes;
    u8      hash[BLOCK_HASH_LENGTH];
};

BPF_PERF_OUTPUT(block_headers);
int trace_block_header(struct pt_regs *ctx) {
    struct BlockHeader header = {};
    void *hash_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &header.height);
    bpf_usdt_readarg(2, ctx, &header.peer_id);
    bpf_usdt_readarg(3, ctx, &header.via_compact_block);
    bpf_usdt_readarg(4, ctx, &hash_pointer);
    bpf_probe_read_user(&header.hash, sizeof(header.hash), hash_pointer);
    block_headers.perf_submit(ctx, &header, sizeof(header));
    return 0;
}

BPF_PERF_OUTPUT(compact_block_reconstructed);
int trace_compact_block_reconstructed(struct pt_regs *ctx) {
    struct CompactBlockReconstructed reconstructed = {};
    void *hash_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &reconstructed.peer_id);
    bpf_usdt_readarg(2, ctx, &reconstructed.prefilled_txn_count);
    bpf_usdt_readarg(3, ctx, &reconstructed.mempool_txn_count);
    bpf_usdt_readarg(4, ctx, &reconstructed.extra_pool_txn_count);
    bpf_usdt_readarg(5, ctx, &reconstructed.requested_txn_count);
    bpf_usdt_readarg(6, ctx, &reconstructed.requested_txn_bytes);
    bpf_usdt_readarg(7, ctx, &hash_pointer);
    bpf_probe_read_user(&reconstructed.hash, sizeof(reconstructed.hash), hash_pointer);
    compact_block_reconstructed.perf_submit(ctx, &reconstructed, sizeof(reconstructed));
    return 0;
}

BPF_PERF_OUTPUT(inbound_messages);
int trace_inbound_message(struct pt_regs *ctx) {
    struct p2p_message msg = {};
    void *paddr = NULL, *pconn_type = NULL, *pmsg_type = NULL, *pmsg = NULL;
    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg(2, ctx, &paddr);
    bpf_probe_read_user_str(&msg.peer_addr, sizeof(msg.peer_addr), paddr);
    bpf_usdt_readarg(3, ctx, &pconn_type);
    bpf_probe_read_user_str(&msg.peer_conn_type, sizeof(msg.peer_conn_type), pconn_type);
    bpf_usdt_readarg(4, ctx, &pmsg_type);
    bpf_probe_read_user_str(&msg.msg_type, sizeof(msg.msg_type), pmsg_type);
    bpf_usdt_readarg(5, ctx, &msg.msg_size);
    bpf_usdt_readarg(6, ctx, &pmsg);
    bpf_probe_read_user(&msg.msg, _TRACEPOINT_TEST_MIN(msg.msg_size, MAX_MSG_DATA_LENGTH), pmsg);
    inbound_messages.perf_submit(ctx, &msg, sizeof(msg));
    return 0;
}

BPF_PERF_OUTPUT(outbound_messages);
int trace_outbound_message(struct pt_regs *ctx) {
    struct p2p_message msg = {};
    void *paddr = NULL, *pconn_type = NULL, *pmsg_type = NULL, *pmsg = NULL;
    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg(2, ctx, &paddr);
    bpf_probe_read_user_str(&msg.peer_addr, sizeof(msg.peer_addr), paddr);
    bpf_usdt_readarg(3, ctx, &pconn_type);
    bpf_probe_read_user_str(&msg.peer_conn_type, sizeof(msg.peer_conn_type), pconn_type);
    bpf_usdt_readarg(4, ctx, &pmsg_type);
    bpf_probe_read_user_str(&msg.msg_type, sizeof(msg.msg_type), pmsg_type);
    bpf_usdt_readarg(5, ctx, &msg.msg_size);
    bpf_usdt_readarg(6, ctx, &pmsg);
    bpf_probe_read_user(&msg.msg, _TRACEPOINT_TEST_MIN(msg.msg_size, MAX_MSG_DATA_LENGTH), pmsg);
    outbound_messages.perf_submit(ctx, &msg, sizeof(msg));
    return 0;
};

BPF_PERF_OUTPUT(inbound_connections);
int trace_inbound_connection(struct pt_regs *ctx) {
    struct NewConnection inbound = {};
    void *conn_type_pointer = NULL, *address_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &inbound.conn.id);
    bpf_usdt_readarg(2, ctx, &address_pointer);
    bpf_usdt_readarg(3, ctx, &conn_type_pointer);
    bpf_usdt_readarg(4, ctx, &inbound.conn.network);
    bpf_usdt_readarg(5, ctx, &inbound.existing);
    bpf_probe_read_user_str(&inbound.conn.addr, sizeof(inbound.conn.addr), address_pointer);
    bpf_probe_read_user_str(&inbound.conn.type, sizeof(inbound.conn.type), conn_type_pointer);
    inbound_connections.perf_submit(ctx, &inbound, sizeof(inbound));
    return 0;
};

BPF_PERF_OUTPUT(outbound_connections);
int trace_outbound_connection(struct pt_regs *ctx) {
    struct NewConnection outbound = {};
    void *conn_type_pointer = NULL, *address_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &outbound.conn.id);
    bpf_usdt_readarg(2, ctx, &address_pointer);
    bpf_usdt_readarg(3, ctx, &conn_type_pointer);
    bpf_usdt_readarg(4, ctx, &outbound.conn.network);
    bpf_usdt_readarg(5, ctx, &outbound.existing);
    bpf_probe_read_user_str(&outbound.conn.addr, sizeof(outbound.conn.addr), address_pointer);
    bpf_probe_read_user_str(&outbound.conn.type, sizeof(outbound.conn.type), conn_type_pointer);
    outbound_connections.perf_submit(ctx, &outbound, sizeof(outbound));
    return 0;
};

BPF_PERF_OUTPUT(evicted_inbound_connections);
int trace_evicted_inbound_connection(struct pt_regs *ctx) {
    struct ClosedConnection evicted = {};
    void *conn_type_pointer = NULL, *address_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &evicted.conn.id);
    bpf_usdt_readarg(2, ctx, &address_pointer);
    bpf_usdt_readarg(3, ctx, &conn_type_pointer);
    bpf_usdt_readarg(4, ctx, &evicted.conn.network);
    bpf_usdt_readarg(5, ctx, &evicted.time_established);
    bpf_probe_read_user_str(&evicted.conn.addr, sizeof(evicted.conn.addr), address_pointer);
    bpf_probe_read_user_str(&evicted.conn.type, sizeof(evicted.conn.type), conn_type_pointer);
    evicted_inbound_connections.perf_submit(ctx, &evicted, sizeof(evicted));
    return 0;
};

BPF_PERF_OUTPUT(misbehaving_connections);
int trace_misbehaving_connection(struct pt_regs *ctx) {
    struct MisbehavingConnection misbehaving = {};
    void *message_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &misbehaving.id);
    bpf_usdt_readarg(2, ctx, &message_pointer);
    bpf_probe_read_user_str(&misbehaving.message, sizeof(misbehaving.message), message_pointer);
    misbehaving_connections.perf_submit(ctx, &misbehaving, sizeof(misbehaving));
    return 0;
};

BPF_PERF_OUTPUT(closed_connections);
int trace_closed_connection(struct pt_regs *ctx) {
    struct ClosedConnection closed = {};
    void *conn_type_pointer = NULL, *address_pointer = NULL;
    bpf_usdt_readarg(1, ctx, &closed.conn.id);
    bpf_usdt_readarg(2, ctx, &address_pointer);
    bpf_usdt_readarg(3, ctx, &conn_type_pointer);
    bpf_usdt_readarg(4, ctx, &closed.conn.network);
    bpf_usdt_readarg(5, ctx, &closed.time_established);
    bpf_probe_read_user_str(&closed.conn.addr, sizeof(closed.conn.addr), address_pointer);
    bpf_probe_read_user_str(&closed.conn.type, sizeof(closed.conn.type), conn_type_pointer);
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
        ("message", ctypes.c_char * MAX_MISBEHAVING_MESSAGE_LENGTH),
    ]

    def __repr__(self):
        return f"MisbehavingConnection(id={self.id}, message={self.message})"


class BlockHeader(ctypes.Structure):
    _fields_ = [
        ("height", ctypes.c_int32),
        ("peer_id", ctypes.c_int64),
        ("via_compact_block", ctypes.c_bool),
        ("hash", ctypes.c_ubyte * BLOCK_HASH_LENGTH),
    ]

    def __repr__(self):
        return f"BlockHeader(hash={bytes(self.hash[::-1]).hex()}, height={self.height}, peer_id={self.peer_id}, via_compact_block={self.via_compact_block})"


class CompactBlockReconstructed(ctypes.Structure):
    _fields_ = [
        ("peer_id", ctypes.c_int64),
        ("prefilled_txn_count", ctypes.c_uint32),
        ("mempool_txn_count", ctypes.c_uint32),
        ("extra_pool_txn_count", ctypes.c_uint32),
        ("requested_txn_count", ctypes.c_uint32),
        ("requested_txn_bytes", ctypes.c_uint32),
        ("hash", ctypes.c_ubyte * BLOCK_HASH_LENGTH),
    ]

    def __repr__(self):
        return f"CompactBlockReconstructed(hash={bytes(self.hash[::-1]).hex()}, peer_id={self.peer_id}, prefilled_txn_count={self.prefilled_txn_count}, mempool_txn_count={self.mempool_txn_count}, extra_pool_txn_count={self.extra_pool_txn_count}, requested_txn_count={self.requested_txn_count}, requested_txn_bytes={self.requested_txn_bytes})"


class NetTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-maxconnections={MAX_CONNECTIONS}']]

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_python_bcc()
        self.skip_if_no_bpf_permissions()
        self.skip_if_running_under_valgrind()

    def run_test(self):
        self.block_header_tracepoint_test()
        self.compact_block_reconstructed_tracepoint_test()
        self.p2p_message_tracepoint_test()
        self.inbound_conn_tracepoint_test()
        self.outbound_conn_tracepoint_test()
        self.evicted_inbound_conn_tracepoint_test()
        self.misbehaving_conn_tracepoint_test()
        self.closed_conn_tracepoint_test()

    def block_header_tracepoint_test(self):
        self.log.info("hook into the net:block_header tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:block_header",
                         fn_name="trace_block_header")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

        block_headers = []

        def handle_block_header(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(BlockHeader)).contents
            self.log.info(f"handle_block_header(): {event}")
            block_headers.append(event)

        bpf["block_headers"].open_perf_buffer(handle_block_header)

        self.log.info("send a new header from a P2P test node to bitcoind")
        test_node = P2PInterface()
        self.nodes[0].add_p2p_connection(test_node)
        peer_id = self.nodes[0].getpeerinfo()[0]["id"]
        expected_height = self.nodes[0].getblockcount() + 1
        block = create_block(tmpl=self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
        block.solve()
        test_node.send_and_ping(msg_headers([CBlockHeader(block)]))
        bpf.perf_buffer_poll(timeout=400)

        matching_headers = [event for event in block_headers if bytes(event.hash[::-1]).hex() == block.hash_hex]
        assert_equal(1, len(matching_headers))
        event = matching_headers[0]
        assert_equal(expected_height, event.height)
        assert_equal(peer_id, event.peer_id)
        assert_equal(False, bool(event.via_compact_block))

        self.log.info("send a cmpctblock with a new header to bitcoind")
        # Build a distinct block at the same height (the previous header has no
        # body, so the chain tip has not advanced). Bumping nTime gives it a
        # different hash so it is treated as a new header.
        cmpct_block = create_block(
            tmpl=self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS),
            ntime=block.nTime + 1,
        )
        cmpct_block.solve()
        compact_block = HeaderAndShortIDs()
        compact_block.initialize_from_block(cmpct_block)
        test_node.send_and_ping(msg_cmpctblock(compact_block.to_p2p()))
        bpf.perf_buffer_poll(timeout=400)

        matching_headers = [event for event in block_headers if bytes(event.hash[::-1]).hex() == cmpct_block.hash_hex]
        assert_equal(1, len(matching_headers))
        event = matching_headers[0]
        assert_equal(expected_height, event.height)
        assert_equal(peer_id, event.peer_id)
        assert_equal(True, bool(event.via_compact_block))

        bpf.cleanup()
        test_node.peer_disconnect()
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 0)

    def compact_block_reconstructed_tracepoint_test(self):
        self.log.info("hook into the net:compact_block_reconstructed tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:compact_block_reconstructed",
                         fn_name="trace_compact_block_reconstructed")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

        reconstructed_blocks = []

        def handle_compact_block_reconstructed(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(CompactBlockReconstructed)).contents
            self.log.info(f"handle_compact_block_reconstructed(): {event}")
            reconstructed_blocks.append(event)

        bpf["compact_block_reconstructed"].open_perf_buffer(handle_compact_block_reconstructed)

        def send_compact_block(block, *, use_witness=False):
            compact_block = HeaderAndShortIDs()
            compact_block.initialize_from_block(block, use_witness=use_witness)
            test_node.send_and_ping(msg_cmpctblock(compact_block.to_p2p()))
            assert_equal(block.hash_hex, self.nodes[0].getbestblockhash())

        def assert_compact_block_reconstructed(
            block_hash,
            *,
            peer_id,
            prefilled_txn_count,
            mempool_txn_count,
            extra_pool_txn_count,
            requested_txn_count,
            requested_txn_bytes,
        ):
            bpf.perf_buffer_poll(timeout=400)
            matching_blocks = [event for event in reconstructed_blocks if bytes(event.hash[::-1]).hex() == block_hash]
            assert_equal(1, len(matching_blocks))
            event = matching_blocks[0]
            assert_equal(peer_id, event.peer_id)
            assert_equal(prefilled_txn_count, event.prefilled_txn_count)
            assert_equal(mempool_txn_count, event.mempool_txn_count)
            assert_equal(extra_pool_txn_count, event.extra_pool_txn_count)
            assert_equal(requested_txn_count, event.requested_txn_count)
            assert_equal(requested_txn_bytes, event.requested_txn_bytes)

        self.log.info("send a compact block with only a prefilled coinbase transaction to bitcoind")
        test_node = P2PInterface()
        self.nodes[0].add_p2p_connection(test_node)
        peer_id = self.nodes[0].getpeerinfo()[0]["id"]
        block = create_block(tmpl=self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS))
        block.solve()
        send_compact_block(block)
        assert_compact_block_reconstructed(
            block.hash_hex,
            peer_id=peer_id,
            prefilled_txn_count=1,
            mempool_txn_count=0,
            extra_pool_txn_count=0,
            requested_txn_count=0,
            requested_txn_bytes=0,
        )

        self.log.info("send a compact block reconstructed using a replaced transaction from the extra transaction pool")
        wallet = MiniWallet(self.nodes[0])
        utxo_to_spend = wallet.get_utxo()
        tx_to_replace = wallet.create_self_transfer(
            fee_rate=Decimal("0.0001"),
            sequence=MAX_BIP125_RBF_SEQUENCE,
            utxo_to_spend=utxo_to_spend,
        )
        tx_replacement = wallet.create_self_transfer(
            fee_rate=Decimal("0.005"),
            sequence=MAX_BIP125_RBF_SEQUENCE,
            utxo_to_spend=utxo_to_spend,
        )
        test_node.send_and_ping(msg_tx(tx_to_replace["tx"]))
        assert_equal([tx_to_replace["txid"]], self.nodes[0].getrawmempool())
        test_node.send_and_ping(msg_tx(tx_replacement["tx"]))
        assert_equal([tx_replacement["txid"]], self.nodes[0].getrawmempool())

        block = create_block(
            tmpl=self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS),
            txlist=[tx_to_replace["tx"]],
        )
        add_witness_commitment(block)
        block.solve()
        send_compact_block(block, use_witness=True)
        assert_compact_block_reconstructed(
            block.hash_hex,
            peer_id=peer_id,
            prefilled_txn_count=1,
            mempool_txn_count=0,
            extra_pool_txn_count=1,
            requested_txn_count=0,
            requested_txn_bytes=0,
        )

        bpf.cleanup()
        test_node.peer_disconnect()
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 0)

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
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

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
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

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
            assert_greater_than(inbound_connection.conn.id, 0)
            assert_greater_than(inbound_connection.existing, 0)
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
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

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
            assert_greater_than(outbound_connection.conn.id, 0)
            assert_greater_than(outbound_connection.existing, 0)
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
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

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
            assert_greater_than(evicted_connection.conn.id, 0)
            assert_greater_than(evicted_connection.time_established, 0)
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
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

        EXPECTED_MISBEHAVING_CONNECTIONS = 2
        misbehaving_connections = []

        def handle_misbehaving_connection(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(MisbehavingConnection)).contents
            self.log.info(f"handle_misbehaving_connection(): {event}")
            misbehaving_connections.append(event)

        bpf["misbehaving_connections"].open_perf_buffer(handle_misbehaving_connection)

        self.log.info("connect a misbehaving P2P test nodes to our bitcoind node")
        msg = msg_headers([CBlockHeader()] * (MAX_HEADERS_RESULTS + 1))
        for _ in range(EXPECTED_MISBEHAVING_CONNECTIONS):
            testnode = P2PInterface()
            self.nodes[0].add_p2p_connection(testnode)
            testnode.send_without_ping(msg)
            bpf.perf_buffer_poll(timeout=500)
            testnode.peer_disconnect()

        assert_equal(EXPECTED_MISBEHAVING_CONNECTIONS, len(misbehaving_connections))
        for misbehaving_connection in misbehaving_connections:
            assert_greater_than(misbehaving_connection.id, 0)
            assert_greater_than(len(misbehaving_connection.message), 0)
            assert_equal(misbehaving_connection.message, b"headers message size = 2001")

        bpf.cleanup()

    def closed_conn_tracepoint_test(self):
        self.log.info("hook into the net:closed_connection tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="net:closed_connection",
                         fn_name="trace_closed_connection")
        bpf = BPF(text=net_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=bpf_cflags())

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
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 0)
        bpf.perf_buffer_poll(timeout=400)

        assert_equal(EXPECTED_CLOSED_CONNECTIONS, len(closed_connections))
        for closed_connection in closed_connections:
            assert_greater_than(closed_connection.conn.id, 0)
            assert_equal("inbound", closed_connection.conn.conn_type.decode('utf-8'))
            assert_equal(0, closed_connection.conn.network)
            assert_greater_than(closed_connection.time_established, 0)

        bpf.cleanup()

if __name__ == '__main__':
    NetTracepointTest(__file__).main()
