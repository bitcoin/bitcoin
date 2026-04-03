#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Record TxGraph operations via USDT tracepoints into a binary trace file.

Requires BCC (BPF Compiler Collection). Must be run as root.
Requires bitcoind started with TXGRAPH_WAIT_FOR_TRACER=1 environment variable
so that bitcoind waits for the tracer to attach before mempool initialization,
ensuring a complete trace starting from the INIT event.

Usage:
    sudo python3 txgraph_trace_recorder.py -p <bitcoind_pid> -o trace.bin

The output trace file uses the TXGTRACE binary format compatible with the
txgraph-replay tool for performance benchmarking.

Variable-length operations (get_ancestors_union, get_descendants_union,
count_distinct_clusters) record up to 64 per-element indices via
bpf_probe_read_user. The C++ side passes a pointer to a fixed-size
stack buffer that the eBPF program reads in full.
"""

import argparse
import ctypes
import signal
import struct

from bcc import BPF, USDT

# Max elements for variable-length operations (matches C++ MAX_TRACE)
MAX_ELEMENTS = 64

# Opcodes matching TxGraphTraceOp in txgraph_tracing.h
OP_INIT                        = 0x00
OP_ADD_TRANSACTION             = 0x01
OP_REMOVE_TRANSACTION          = 0x02
OP_ADD_DEPENDENCY              = 0x03
OP_SET_TRANSACTION_FEE         = 0x04
OP_UNLINK_REF                  = 0x05
OP_GET_BLOCK_BUILDER           = 0x10
OP_DO_WORK                     = 0x11
OP_COMPARE_MAIN_ORDER          = 0x12
OP_GET_ANCESTORS               = 0x13
OP_GET_DESCENDANTS             = 0x14
OP_GET_ANCESTORS_UNION         = 0x15
OP_GET_DESCENDANTS_UNION       = 0x16
OP_GET_CLUSTER                 = 0x17
OP_GET_MAIN_CHUNK_FEERATE      = 0x18
OP_COUNT_DISTINCT_CLUSTERS     = 0x19
OP_GET_MAIN_MEMORY_USAGE       = 0x1a
OP_GET_WORST_MAIN_CHUNK        = 0x1b
OP_GET_MAIN_STAGING_DIAGRAMS   = 0x1c
OP_TRIM                        = 0x1d
OP_IS_OVERSIZED                = 0x1e
OP_EXISTS                      = 0x1f
OP_START_STAGING               = 0x20
OP_ABORT_STAGING               = 0x21
OP_COMMIT_STAGING              = 0x22
OP_GET_INDIVIDUAL_FEERATE      = 0x23
OP_GET_TRANSACTION_COUNT       = 0x24

# Probe definitions: (name, opcode, nargs, varlen)
# - nargs: number of fixed USDT arguments (0-3)
# - varlen: if True, arg3 is a pointer to a uint32_t[64] buffer read via
#   bpf_probe_read_user (for variable-length operations)
PROBES = [
    ("init",                        0x00, 3, False),
    ("add_transaction",             0x01, 3, False),
    ("remove_transaction",          0x02, 1, False),
    ("add_dependency",              0x03, 2, False),
    ("set_transaction_fee",         0x04, 2, False),
    ("unlink_ref",                  0x05, 1, False),
    ("get_block_builder",           0x10, 0, False),
    ("do_work",                     0x11, 1, False),
    ("compare_main_order",          0x12, 2, False),
    ("get_ancestors",               0x13, 2, False),
    ("get_descendants",             0x14, 2, False),
    ("get_ancestors_union",         0x15, 2, True),
    ("get_descendants_union",       0x16, 2, True),
    ("get_cluster",                 0x17, 2, False),
    ("get_main_chunk_feerate",      0x18, 1, False),
    ("count_distinct_clusters",     0x19, 2, True),
    ("get_main_memory_usage",       0x1a, 0, False),
    ("get_worst_main_chunk",        0x1b, 0, False),
    ("get_main_staging_diagrams",   0x1c, 0, False),
    ("trim",                        0x1d, 0, False),
    ("is_oversized",                0x1e, 1, False),
    ("exists",                      0x1f, 2, False),
    ("start_staging",               0x20, 0, False),
    ("abort_staging",               0x21, 0, False),
    ("commit_staging",              0x22, 0, False),
    ("get_individual_feerate",      0x23, 1, False),
    ("get_transaction_count",       0x24, 1, False),
]


def generate_bpf_program():
    """Generate the eBPF C program with one handler function per probe.

    BCC does not allow its builtins (bpf_usdt_readarg, perf_submit) inside
    C preprocessor macro expansions.  Instead of C macros, each handler is
    emitted as a standalone C function from Python.
    """
    arg_fields = ["arg0", "arg1", "arg2"]
    lines = [
        "#include <uapi/linux/ptrace.h>",
        "",
        "#define MAX_ELEMENTS 64",
        "",
        "struct txgraph_event {",
        "    u8  opcode;",
        "    u64 arg0;",
        "    u64 arg1;",
        "    u64 arg2;",
        "    u32 elements[MAX_ELEMENTS];",
        "};",
        "",
        "BPF_PERF_OUTPUT(events);",
        "",
    ]
    for name, opcode, nargs, varlen in PROBES:
        lines.append(f"int trace_{name}(struct pt_regs *ctx) {{")
        lines.append("    struct txgraph_event e = {};")
        lines.append(f"    e.opcode = {opcode:#04x};")
        for i in range(nargs):
            lines.append(f"    bpf_usdt_readarg({i+1}, ctx, &e.{arg_fields[i]});")
        if varlen:
            lines.append("    u64 ptr = 0;")
            lines.append("    bpf_usdt_readarg(3, ctx, &ptr);")
            lines.append("    if (ptr != 0) {")
            lines.append("        bpf_probe_read_user(e.elements,")
            lines.append("                            sizeof(e.elements),")
            lines.append("                            (void*)ptr);")
            lines.append("    }")
        lines.append("    events.perf_submit(ctx, &e, sizeof(e));")
        lines.append("    return 0;")
        lines.append("}")
        lines.append("")
    return "\n".join(lines)


class TxGraphEvent(ctypes.Structure):
    _fields_ = [
        ("opcode",   ctypes.c_uint8),
        ("arg0",     ctypes.c_uint64),
        ("arg1",     ctypes.c_uint64),
        ("arg2",     ctypes.c_uint64),
        ("elements", ctypes.c_uint32 * MAX_ELEMENTS),
    ]


def write_u8(f, v):
    f.write(struct.pack("<B", v))

def write_u32(f, v):
    f.write(struct.pack("<I", v & 0xFFFFFFFF))

def write_i32(f, v):
    f.write(struct.pack("<i", ctypes.c_int32(v).value))

def write_u64(f, v):
    f.write(struct.pack("<Q", v))

def write_i64(f, v):
    f.write(struct.pack("<q", ctypes.c_int64(v).value))


def write_event(f, e):
    """Write one trace event in TXGTRACE binary format."""
    op = e.opcode
    write_u8(f, op)

    if op == OP_INIT:
        write_u32(f, e.arg0)        # max_cluster_count
        write_u64(f, e.arg1)        # max_cluster_size
        write_u64(f, e.arg2)        # acceptable_cost
    elif op == OP_ADD_TRANSACTION:
        write_u32(f, e.arg0)        # graph_idx
        write_i64(f, e.arg1)        # fee
        write_i32(f, e.arg2)        # size
    elif op == OP_REMOVE_TRANSACTION:
        write_u32(f, e.arg0)        # graph_idx
    elif op == OP_ADD_DEPENDENCY:
        write_u32(f, e.arg0)        # parent
        write_u32(f, e.arg1)        # child
    elif op == OP_SET_TRANSACTION_FEE:
        write_u32(f, e.arg0)        # graph_idx
        write_i64(f, e.arg1)        # fee
    elif op == OP_UNLINK_REF:
        write_u32(f, e.arg0)        # graph_idx
    elif op == OP_DO_WORK:
        write_u64(f, e.arg0)        # max_cost
    elif op == OP_COMPARE_MAIN_ORDER:
        write_u32(f, e.arg0)        # idx_a
        write_u32(f, e.arg1)        # idx_b
    elif op in (OP_GET_ANCESTORS, OP_GET_DESCENDANTS, OP_GET_CLUSTER, OP_EXISTS):
        write_u32(f, e.arg0)        # idx
        write_u8(f, e.arg1)         # level
    elif op in (OP_GET_ANCESTORS_UNION, OP_GET_DESCENDANTS_UNION, OP_COUNT_DISTINCT_CLUSTERS):
        count = e.arg0 & 0xFFFFFFFF
        write_u32(f, count)         # count
        write_u8(f, e.arg1)         # level
        for i in range(min(count, MAX_ELEMENTS)):
            write_u32(f, e.elements[i])
    elif op in (OP_GET_MAIN_CHUNK_FEERATE, OP_GET_INDIVIDUAL_FEERATE):
        write_u32(f, e.arg0)        # idx
    elif op in (OP_IS_OVERSIZED, OP_GET_TRANSACTION_COUNT):
        write_u8(f, e.arg0)         # level
    # All other ops have no payload


def main():
    parser = argparse.ArgumentParser(
        description="Record TxGraph USDT tracepoint events to a binary trace file.")
    parser.add_argument("-p", "--pid", type=int, required=True,
                        help="PID of the bitcoind process")
    parser.add_argument("-o", "--output", type=str, required=True,
                        help="Output trace file path")
    args = parser.parse_args()

    usdt = USDT(pid=args.pid)
    for name, opcode, nargs, varlen in PROBES:
        usdt.enable_probe(probe=f"txgraph:{name}", fn_name=f"trace_{name}")

    bpf = BPF(text=generate_bpf_program(), usdt_contexts=[usdt])

    outf = open(args.output, "wb")
    # Write TXGTRACE header
    outf.write(b"TXGTRACE")
    write_u32(outf, 1)  # version

    event_count = 0

    def handle_event(cpu, data, size):
        nonlocal event_count
        e = ctypes.cast(data, ctypes.POINTER(TxGraphEvent)).contents
        write_event(outf, e)
        event_count += 1

        if event_count % 1000 == 0:
            outf.flush()

    bpf["events"].open_perf_buffer(handle_event, page_cnt=256)

    print(f"Recording TxGraph trace from PID {args.pid} to {args.output}")
    print("Press Ctrl+C to stop...")

    running = True
    def stop(signum, frame):
        nonlocal running
        running = False
    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    while running:
        try:
            bpf.perf_buffer_poll(timeout=100)
        except KeyboardInterrupt:
            break

    outf.flush()
    outf.close()
    print(f"\nRecording stopped. {event_count} events written to {args.output}")


if __name__ == "__main__":
    main()
