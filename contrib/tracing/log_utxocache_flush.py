#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import ctypes
from bcc import BPF, USDT

"""Example logging Bitcoin Core utxo set cache flushes utilizing
    the utxocache:flush tracepoint."""

# USAGE:  ./contrib/tracing/log_utxocache_flush.py path/to/bitcoind

# BCC: The C program to be compiled to an eBPF program (by BCC) and loaded into
# a sandboxed Linux kernel VM.
program = """
# include <uapi/linux/ptrace.h>

struct data_t
{
  u64 duration;
  u32 mode;
  u64 coins_count;
  u64 coins_mem_usage;
  bool is_flush_for_prune;
};

// BPF perf buffer to push the data to user space.
BPF_PERF_OUTPUT(flush);

int trace_flush(struct pt_regs *ctx) {
  struct data_t data = {};
  bpf_usdt_readarg(1, ctx, &data.duration);
  bpf_usdt_readarg(2, ctx, &data.mode);
  bpf_usdt_readarg(3, ctx, &data.coins_count);
  bpf_usdt_readarg(4, ctx, &data.coins_mem_usage);
  bpf_usdt_readarg(5, ctx, &data.is_flush_for_prune);
  flush.perf_submit(ctx, &data, sizeof(data));
  return 0;
}
"""

FLUSH_MODES = [
    'NONE',
    'IF_NEEDED',
    'PERIODIC',
    'ALWAYS'
]


class Data(ctypes.Structure):
    # define output data structure corresponding to struct data_t
    _fields_ = [
        ("duration", ctypes.c_uint64),
        ("mode", ctypes.c_uint32),
        ("coins_count", ctypes.c_uint64),
        ("coins_mem_usage", ctypes.c_uint64),
        ("is_flush_for_prune", ctypes.c_bool)
    ]


def print_event(event):
    print("%-15d %-10s %-15d %-15s %-8s" % (
        event.duration,
        FLUSH_MODES[event.mode],
        event.coins_count,
        "%.2f kB" % (event.coins_mem_usage/1000),
        event.is_flush_for_prune
    ))


def main(pid):
    print(f"Hooking into bitcoind with pid {pid}")
    bitcoind_with_usdts = USDT(pid=int(pid))

    # attaching the trace functions defined in the BPF program
    # to the tracepoints
    bitcoind_with_usdts.enable_probe(
        probe="flush", fn_name="trace_flush")
    b = BPF(text=program, usdt_contexts=[bitcoind_with_usdts])

    def handle_flush(_, data, size):
        """ Coins Flush handler.
          Called each time coin caches and indexes are flushed."""
        event = ctypes.cast(data, ctypes.POINTER(Data)).contents
        print_event(event)

    b["flush"].open_perf_buffer(handle_flush)
    print("Logging utxocache flushes. Ctrl-C to end...")
    print("%-15s %-10s %-15s %-15s %-8s" % ("Duration (µs)", "Mode",
                                            "Coins Count", "Memory Usage",
                                            "Flush for Prune"))

    while True:
        try:
            b.perf_buffer_poll()
        except KeyboardInterrupt:
            exit(0)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE: ", sys.argv[0], "<pid of bitcoind>")
        exit(1)

    pid = sys.argv[1]
    main(pid)
