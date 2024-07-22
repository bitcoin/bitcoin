#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Tests the validation:* tracepoint API interface.
    See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-validation
"""

import ctypes

# Test will be skipped if we don't have bcc installed
try:
    from bcc import BPF, USDT # type: ignore[import]
except ImportError:
    pass

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


validation_blockconnected_program = """
#include <uapi/linux/ptrace.h>

typedef signed long long i64;

struct connected_block
{
    char        hash[32];
    int         height;
    i64         transactions;
    int         inputs;
    i64         sigops;
    u64         duration;
};

BPF_PERF_OUTPUT(block_connected);
int trace_block_connected(struct pt_regs *ctx) {
    struct connected_block block = {};
    bpf_usdt_readarg_p(1, ctx, &block.hash, 32);
    bpf_usdt_readarg(2, ctx, &block.height);
    bpf_usdt_readarg(3, ctx, &block.transactions);
    bpf_usdt_readarg(4, ctx, &block.inputs);
    bpf_usdt_readarg(5, ctx, &block.sigops);
    bpf_usdt_readarg(6, ctx, &block.duration);
    block_connected.perf_submit(ctx, &block, sizeof(block));
    return 0;
}
"""


class ValidationTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_python_bcc()
        self.skip_if_no_bpf_permissions()

    def run_test(self):
        # Tests the validation:block_connected tracepoint by generating blocks
        # and comparing the values passed in the tracepoint arguments with the
        # blocks.
        # See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#tracepoint-validationblock_connected

        class Block(ctypes.Structure):
            _fields_ = [
                ("hash", ctypes.c_ubyte * 32),
                ("height", ctypes.c_int),
                ("transactions", ctypes.c_int64),
                ("inputs", ctypes.c_int),
                ("sigops", ctypes.c_int64),
                ("duration", ctypes.c_uint64),
            ]

            def __repr__(self):
                return "ConnectedBlock(hash=%s height=%d, transactions=%d, inputs=%d, sigops=%d, duration=%d)" % (
                    bytes(self.hash[::-1]).hex(),
                    self.height,
                    self.transactions,
                    self.inputs,
                    self.sigops,
                    self.duration)

        BLOCKS_EXPECTED = 2
        expected_blocks = dict()
        events = []

        self.log.info("hook into the validation:block_connected tracepoint")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="validation:block_connected",
                         fn_name="trace_block_connected")
        bpf = BPF(text=validation_blockconnected_program,
                  usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        def handle_blockconnected(_, data, __):
            event = ctypes.cast(data, ctypes.POINTER(Block)).contents
            self.log.info(f"handle_blockconnected(): {event}")
            events.append(event)

        bpf["block_connected"].open_perf_buffer(
            handle_blockconnected)

        self.log.info(f"mine {BLOCKS_EXPECTED} blocks")
        block_hashes = self.generatetoaddress(
            self.nodes[0], BLOCKS_EXPECTED, ADDRESS_BCRT1_UNSPENDABLE)
        for block_hash in block_hashes:
            expected_blocks[block_hash] = self.nodes[0].getblock(block_hash, 2)

        bpf.perf_buffer_poll(timeout=200)

        self.log.info(f"check that we correctly traced {BLOCKS_EXPECTED} blocks")
        for event in events:
            block_hash = bytes(event.hash[::-1]).hex()
            block = expected_blocks[block_hash]
            assert_equal(block["hash"], block_hash)
            assert_equal(block["height"], event.height)
            assert_equal(len(block["tx"]), event.transactions)
            assert_equal(len([tx["vin"] for tx in block["tx"]]), event.inputs)
            assert_equal(0, event.sigops)  # no sigops in coinbase tx
            # only plausibility checks
            assert event.duration > 0
            del expected_blocks[block_hash]
        assert_equal(BLOCKS_EXPECTED, len(events))
        assert_equal(0, len(expected_blocks))

        bpf.cleanup()


if __name__ == '__main__':
    ValidationTracepointTest(__file__).main()
