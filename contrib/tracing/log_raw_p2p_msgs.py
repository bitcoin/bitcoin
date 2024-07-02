#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Demonstration of eBPF limitations and the effect on USDT with the
    net:inbound_message and net:outbound_message tracepoints. """

# This script shows a limitation of eBPF when data larger than 32kb is passed to
# user-space. It uses BCC (https://github.com/iovisor/bcc) to load a sandboxed
# eBPF program into the Linux kernel (root privileges are required). The eBPF
# program attaches to two statically defined tracepoints. The tracepoint
# 'net:inbound_message' is called when a new P2P message is received, and
# 'net:outbound_message' is called on outbound P2P messages. The eBPF program
# submits the P2P messages to this script via a BPF ring buffer. The submitted
# messages are printed.

# eBPF Limitations:
#
# Bitcoin P2P messages can be larger than 32kb (e.g. tx, block, ...). The eBPF
# VM's stack is limited to 512 bytes, and we can't allocate more than about 32kb
# for a P2P message in the eBPF VM. The message data is cut off when the message
# is larger than MAX_MSG_DATA_LENGTH (see definition below). This can be detected
# in user-space by comparing the data length to the message length variable. The
# message is cut off when the data length is smaller than the message length.
# A warning is included with the printed message data.
#
# Data is submitted to user-space (i.e. to this script) via a ring buffer. The
# throughput of the ring buffer is limited. Each p2p_message is about 32kb in
# size. In- or outbound messages submitted to the ring buffer in rapid
# succession fill the ring buffer faster than it can be read. Some messages are
# lost.
#
# BCC prints: "Possibly lost 2 samples" on lost messages.

import sys
from bcc import BPF, USDT

# BCC: The C program to be compiled to an eBPF program (by BCC) and loaded into
# a sandboxed Linux kernel VM.
program = """
#include <uapi/linux/ptrace.h>

#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

// Maximum possible allocation size
// from include/linux/percpu.h in the Linux kernel
#define PCPU_MIN_UNIT_SIZE (32 << 10)

// Tor v3 addresses are 62 chars + 6 chars for the port (':12345').
#define MAX_PEER_ADDR_LENGTH 62 + 6
#define MAX_PEER_CONN_TYPE_LENGTH 20
#define MAX_MSG_TYPE_LENGTH 20
#define MAX_MSG_DATA_LENGTH PCPU_MIN_UNIT_SIZE - 200

struct p2p_message
{
    u64     peer_id;
    char    peer_addr[MAX_PEER_ADDR_LENGTH];
    char    peer_conn_type[MAX_PEER_CONN_TYPE_LENGTH];
    char    msg_type[MAX_MSG_TYPE_LENGTH];
    u64     msg_size;
    u8      msg[MAX_MSG_DATA_LENGTH];
};

// We can't store the p2p_message struct on the eBPF stack as it is limited to
// 512 bytes and P2P message can be bigger than 512 bytes. However, we can use
// an BPF-array with a length of 1 to allocate up to 32768 bytes (this is
// defined by PCPU_MIN_UNIT_SIZE in include/linux/percpu.h in the Linux kernel).
// Also see https://github.com/iovisor/bcc/issues/2306
BPF_ARRAY(msg_arr, struct p2p_message, 1);

// Two BPF perf buffers for pushing data (here P2P messages) to user-space.
BPF_PERF_OUTPUT(inbound_messages);
BPF_PERF_OUTPUT(outbound_messages);

int trace_inbound_message(struct pt_regs *ctx) {
    int idx = 0;
    struct p2p_message *msg = msg_arr.lookup(&idx);

    // lookup() does not return a NULL pointer. However, the BPF verifier
    // requires an explicit check that that the `msg` pointer isn't a NULL
    // pointer. See https://github.com/iovisor/bcc/issues/2595
    if (msg == NULL) return 1;

    bpf_usdt_readarg(1, ctx, &msg->peer_id);
    bpf_usdt_readarg_p(2, ctx, &msg->peer_addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &msg->peer_conn_type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg_p(4, ctx, &msg->msg_type, MAX_MSG_TYPE_LENGTH);
    bpf_usdt_readarg(5, ctx, &msg->msg_size);
    bpf_usdt_readarg_p(6, ctx, &msg->msg, MIN(msg->msg_size, MAX_MSG_DATA_LENGTH));

    inbound_messages.perf_submit(ctx, msg, sizeof(*msg));
    return 0;
};

int trace_outbound_message(struct pt_regs *ctx) {
    int idx = 0;
    struct p2p_message *msg = msg_arr.lookup(&idx);

    // lookup() does not return a NULL pointer. However, the BPF verifier
    // requires an explicit check that that the `msg` pointer isn't a NULL
    // pointer. See https://github.com/iovisor/bcc/issues/2595
    if (msg == NULL) return 1;

    bpf_usdt_readarg(1, ctx, &msg->peer_id);
    bpf_usdt_readarg_p(2, ctx, &msg->peer_addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &msg->peer_conn_type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg_p(4, ctx, &msg->msg_type, MAX_MSG_TYPE_LENGTH);
    bpf_usdt_readarg(5, ctx, &msg->msg_size);
    bpf_usdt_readarg_p(6, ctx, &msg->msg,  MIN(msg->msg_size, MAX_MSG_DATA_LENGTH));

    outbound_messages.perf_submit(ctx, msg, sizeof(*msg));
    return 0;
};
"""


def print_message(event, inbound):
    print(f"%s %s msg '%s' from peer %d (%s, %s) with %d bytes: %s" %
          (
              f"Warning: incomplete message (only %d out of %d bytes)!" % (
                  len(event.msg), event.msg_size) if len(event.msg) < event.msg_size else "",
              "inbound" if inbound else "outbound",
              event.msg_type.decode("utf-8"),
              event.peer_id,
              event.peer_conn_type.decode("utf-8"),
              event.peer_addr.decode("utf-8"),
              event.msg_size,
              bytes(event.msg[:event.msg_size]).hex(),
          )
          )


def main(pid):
    print(f"Hooking into bitcoind with pid {pid}")
    bitcoind_with_usdts = USDT(pid=int(pid))

    # attaching the trace functions defined in the BPF program to the tracepoints
    bitcoind_with_usdts.enable_probe(
        probe="inbound_message", fn_name="trace_inbound_message")
    bitcoind_with_usdts.enable_probe(
        probe="outbound_message", fn_name="trace_outbound_message")
    bpf = BPF(text=program, usdt_contexts=[bitcoind_with_usdts])

    # BCC: perf buffer handle function for inbound_messages
    def handle_inbound(_, data, size):
        """ Inbound message handler.

        Called each time a message is submitted to the inbound_messages BPF table."""

        event = bpf["inbound_messages"].event(data)
        print_message(event, True)

    # BCC: perf buffer handle function for outbound_messages

    def handle_outbound(_, data, size):
        """ Outbound message handler.

        Called each time a message is submitted to the outbound_messages BPF table."""

        event = bpf["outbound_messages"].event(data)
        print_message(event, False)

    # BCC: add handlers to the inbound and outbound perf buffers
    bpf["inbound_messages"].open_perf_buffer(handle_inbound)
    bpf["outbound_messages"].open_perf_buffer(handle_outbound)

    print("Logging raw P2P messages.")
    print("Messages larger that about 32kb will be cut off!")
    print("Some messages might be lost!")
    while True:
        try:
            bpf.perf_buffer_poll()
        except KeyboardInterrupt:
            exit()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE:", sys.argv[0], "<pid of bitcoind>")
        exit()
    pid = sys.argv[1]
    main(pid)
