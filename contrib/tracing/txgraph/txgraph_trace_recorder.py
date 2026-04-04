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
count_distinct_clusters) record per-element indices via bpf_probe_read_user.
The C++ side passes a pointer to a fixed-size uint64_t stack buffer for each
array argument. Multiple array args per probe are supported: each array gets
a full MAX_ELEMENTS (64) u64 slot range in the eBPF elements buffer.
"""

import argparse
import ctypes
import signal
import struct

from bcc import BPF, USDT
from tracing_defs import PROBES, MAX_ELEMENTS, ELEMENTS_BUFFER_SIZE, write_event, _is_array_type, _count_arrays


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
        f"#define MAX_ELEMENTS {MAX_ELEMENTS}",
        f"#define ELEMENTS_BUFFER_SIZE {ELEMENTS_BUFFER_SIZE}",
        "",
        "struct txgraph_event {",
        "    u8  opcode;",
        "    u64 arg0;",
        "    u64 arg1;",
        "    u64 arg2;",
        "    u64 elements[ELEMENTS_BUFFER_SIZE];",
        "};",
        "",
        "BPF_PERF_OUTPUT(events);",
        "",
        "// Per-CPU array map to hold the event struct off-stack.",
        "// The struct exceeds the 512-byte eBPF stack limit when",
        "// elements use u64 (64 * 8 = 512 bytes for elements alone).",
        "BPF_PERCPU_ARRAY(event_buf, struct txgraph_event, 1);",
        "",
    ]
    for name, opcode, args in PROBES:
        nargs = sum(1 for _, t in args if not _is_array_type(t))
        num_arrays = _count_arrays(args)
        lines.append(f"int trace_{name}(struct pt_regs *ctx) {{")
        lines.append("    int zero = 0;")
        lines.append("    struct txgraph_event *e = event_buf.lookup(&zero);")
        lines.append("    if (!e) return 0;")
        lines.append("    __builtin_memset(e, 0, sizeof(*e));")
        lines.append(f"    e->opcode = {opcode:#04x};")
        for i in range(nargs):
            lines.append(f"    bpf_usdt_readarg({i+1}, ctx, &e->{arg_fields[i]});")
        if num_arrays > 0:
            # Each array arg is passed as a USDT pointer argument following
            # the scalar args.  Each array gets a full MAX_ELEMENTS slot:
            # array k occupies elements[k*MAX_ELEMENTS .. (k+1)*MAX_ELEMENTS-1].
            for arr_idx in range(num_arrays):
                usdt_pos = nargs + 1 + arr_idx
                offset = arr_idx * MAX_ELEMENTS
                lines.append(f"    u64 ptr_{arr_idx} = 0;")
                lines.append(f"    bpf_usdt_readarg({usdt_pos}, ctx, &ptr_{arr_idx});")
                lines.append(f"    if (ptr_{arr_idx} != 0) {{")
                lines.append(f"        bpf_probe_read_user(&e->elements[{offset}],")
                lines.append("                            MAX_ELEMENTS * sizeof(u64),")
                lines.append(f"                            (void*)ptr_{arr_idx});")
                lines.append("    }")
        lines.append("    events.perf_submit(ctx, e, sizeof(*e));")
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
        ("elements", ctypes.c_uint64 * ELEMENTS_BUFFER_SIZE),
    ]


def main():
    parser = argparse.ArgumentParser(
        description="Record TxGraph USDT tracepoint events to a binary trace file.")
    parser.add_argument("-p", "--pid", type=int, required=True,
                        help="PID of the bitcoind process")
    parser.add_argument("-o", "--output", type=str, required=True,
                        help="Output trace file path")
    args = parser.parse_args()

    usdt = USDT(pid=args.pid)
    for name, opcode, _ in PROBES:
        usdt.enable_probe(probe=f"txgraph:{name}", fn_name=f"trace_{name}")

    bpf = BPF(text=generate_bpf_program(), usdt_contexts=[usdt])

    outf = open(args.output, "wb")
    # Write TXGTRACE header
    outf.write(b"TXGTRACE")
    outf.write(struct.pack("<I", 1))  # version

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
