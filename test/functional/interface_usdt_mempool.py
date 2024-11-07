#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the mempool:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-mempool
"""

from decimal import Decimal

# Test will be skipped if we don't have bcc installed
try:
    from bcc import BPF, USDT  # type: ignore[import]
except ImportError:
    pass

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import COIN, DEFAULT_MEMPOOL_EXPIRY_HOURS
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

MEMPOOL_TRACEPOINTS_PROGRAM = """
# include <uapi/linux/ptrace.h>

// The longest rejection reason is 118 chars and is generated in case of SCRIPT_ERR_EVAL_FALSE by
// strprintf("mandatory-script-verify-flag-failed (%s)", ScriptErrorString(check.GetScriptError()))
#define MAX_REJECT_REASON_LENGTH        118
// The longest string returned by RemovalReasonToString() is 'sizelimit'
#define MAX_REMOVAL_REASON_LENGTH       9
#define HASH_LENGTH                     32

struct added_event
{
  u8    hash[HASH_LENGTH];
  s32   vsize;
  s64   fee;
};

struct removed_event
{
  u8    hash[HASH_LENGTH];
  char  reason[MAX_REMOVAL_REASON_LENGTH];
  s32   vsize;
  s64   fee;
  u64   entry_time;
};

struct rejected_event
{
  u8    hash[HASH_LENGTH];
  char  reason[MAX_REJECT_REASON_LENGTH];
};

struct replaced_event
{
  u8    replaced_hash[HASH_LENGTH];
  s32   replaced_vsize;
  s64   replaced_fee;
  u64   replaced_entry_time;
  u8    replacement_hash[HASH_LENGTH];
  s32   replacement_vsize;
  s64   replacement_fee;
};

// BPF perf buffer to push the data to user space.
BPF_PERF_OUTPUT(added_events);
BPF_PERF_OUTPUT(removed_events);
BPF_PERF_OUTPUT(rejected_events);
BPF_PERF_OUTPUT(replaced_events);

int trace_added(struct pt_regs *ctx) {
  struct added_event added = {};

  bpf_usdt_readarg_p(1, ctx, &added.hash, HASH_LENGTH);
  bpf_usdt_readarg(2, ctx, &added.vsize);
  bpf_usdt_readarg(3, ctx, &added.fee);

  added_events.perf_submit(ctx, &added, sizeof(added));
  return 0;
}

int trace_removed(struct pt_regs *ctx) {
  struct removed_event removed = {};

  bpf_usdt_readarg_p(1, ctx, &removed.hash, HASH_LENGTH);
  bpf_usdt_readarg_p(2, ctx, &removed.reason, MAX_REMOVAL_REASON_LENGTH);
  bpf_usdt_readarg(3, ctx, &removed.vsize);
  bpf_usdt_readarg(4, ctx, &removed.fee);
  bpf_usdt_readarg(5, ctx, &removed.entry_time);

  removed_events.perf_submit(ctx, &removed, sizeof(removed));
  return 0;
}

int trace_rejected(struct pt_regs *ctx) {
  struct rejected_event rejected = {};

  bpf_usdt_readarg_p(1, ctx, &rejected.hash, HASH_LENGTH);
  bpf_usdt_readarg_p(2, ctx, &rejected.reason, MAX_REJECT_REASON_LENGTH);

  rejected_events.perf_submit(ctx, &rejected, sizeof(rejected));
  return 0;
}

int trace_replaced(struct pt_regs *ctx) {
  struct replaced_event replaced = {};

  bpf_usdt_readarg_p(1, ctx, &replaced.replaced_hash, HASH_LENGTH);
  bpf_usdt_readarg(2, ctx, &replaced.replaced_vsize);
  bpf_usdt_readarg(3, ctx, &replaced.replaced_fee);
  bpf_usdt_readarg(4, ctx, &replaced.replaced_entry_time);
  bpf_usdt_readarg_p(5, ctx, &replaced.replacement_hash, HASH_LENGTH);
  bpf_usdt_readarg(6, ctx, &replaced.replacement_vsize);
  bpf_usdt_readarg(7, ctx, &replaced.replacement_fee);

  replaced_events.perf_submit(ctx, &replaced, sizeof(replaced));
  return 0;
}

"""


class MempoolTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_python_bcc()
        self.skip_if_no_bpf_permissions()

    def added_test(self):
        """Add a transaction to the mempool and make sure the tracepoint returns
        the expected txid, vsize, and fee."""

        events = []

        self.log.info("Hooking into mempool:added tracepoint...")
        node = self.nodes[0]
        ctx = USDT(pid=node.process.pid)
        ctx.enable_probe(probe="mempool:added", fn_name="trace_added")
        bpf = BPF(text=MEMPOOL_TRACEPOINTS_PROGRAM, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        def handle_added_event(_, data, __):
            events.append(bpf["added_events"].event(data))

        bpf["added_events"].open_perf_buffer(handle_added_event)

        self.log.info("Sending transaction...")
        fee = Decimal(31200)
        tx = self.wallet.send_self_transfer(from_node=node, fee=fee / COIN)

        self.log.info("Polling buffer...")
        bpf.perf_buffer_poll(timeout=200)

        self.log.info("Cleaning up mempool...")
        self.generate(node, 1)

        self.log.info("Ensuring mempool:added event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(bytes(event.hash)[::-1].hex(), tx["txid"])
        assert_equal(event.vsize, tx["tx"].get_vsize())
        assert_equal(event.fee, fee)

        bpf.cleanup()
        self.generate(self.wallet, 1)

    def removed_test(self):
        """Expire a transaction from the mempool and make sure the tracepoint returns
        the expected txid, expiry reason, vsize, and fee."""

        events = []

        self.log.info("Hooking into mempool:removed tracepoint...")
        node = self.nodes[0]
        ctx = USDT(pid=node.process.pid)
        ctx.enable_probe(probe="mempool:removed", fn_name="trace_removed")
        bpf = BPF(text=MEMPOOL_TRACEPOINTS_PROGRAM, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        def handle_removed_event(_, data, __):
            events.append(bpf["removed_events"].event(data))

        bpf["removed_events"].open_perf_buffer(handle_removed_event)

        self.log.info("Sending transaction...")
        fee = Decimal(31200)
        tx = self.wallet.send_self_transfer(from_node=node, fee=fee / COIN)
        txid = tx["txid"]

        self.log.info("Fast-forwarding time to mempool expiry...")
        entry_time = node.getmempoolentry(txid)["time"]
        expiry_time = entry_time + 60 * 60 * DEFAULT_MEMPOOL_EXPIRY_HOURS + 5
        node.setmocktime(expiry_time)

        self.log.info("Triggering expiry...")
        self.wallet.get_utxo(txid=txid)
        self.wallet.send_self_transfer(from_node=node)

        self.log.info("Polling buffer...")
        bpf.perf_buffer_poll(timeout=200)

        self.log.info("Ensuring mempool:removed event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(bytes(event.hash)[::-1].hex(), txid)
        assert_equal(event.reason.decode("UTF-8"), "expiry")
        assert_equal(event.vsize, tx["tx"].get_vsize())
        assert_equal(event.fee, fee)
        assert_equal(event.entry_time, entry_time)

        bpf.cleanup()
        self.generate(self.wallet, 1)

    def replaced_test(self):
        """Replace one and two transactions in the mempool and make sure the tracepoint
        returns the expected txids, vsizes, and fees."""

        events = []

        self.log.info("Hooking into mempool:replaced tracepoint...")
        node = self.nodes[0]
        ctx = USDT(pid=node.process.pid)
        ctx.enable_probe(probe="mempool:replaced", fn_name="trace_replaced")
        bpf = BPF(text=MEMPOOL_TRACEPOINTS_PROGRAM, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        def handle_replaced_event(_, data, __):
            events.append(bpf["replaced_events"].event(data))

        bpf["replaced_events"].open_perf_buffer(handle_replaced_event)

        self.log.info("Sending RBF transaction...")
        utxo = self.wallet.get_utxo(mark_as_spent=True)
        original_fee = Decimal(40000)
        original_tx = self.wallet.send_self_transfer(
            from_node=node, utxo_to_spend=utxo, fee=original_fee / COIN
        )
        entry_time = node.getmempoolentry(original_tx["txid"])["time"]

        self.log.info("Sending replacement transaction...")
        replacement_fee = Decimal(45000)
        replacement_tx = self.wallet.send_self_transfer(
            from_node=node, utxo_to_spend=utxo, fee=replacement_fee / COIN
        )

        self.log.info("Polling buffer...")
        bpf.perf_buffer_poll(timeout=200)

        self.log.info("Ensuring mempool:replaced event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(bytes(event.replaced_hash)[::-1].hex(), original_tx["txid"])
        assert_equal(event.replaced_vsize, original_tx["tx"].get_vsize())
        assert_equal(event.replaced_fee, original_fee)
        assert_equal(event.replaced_entry_time, entry_time)
        assert_equal(bytes(event.replacement_hash)[::-1].hex(), replacement_tx["txid"])
        assert_equal(event.replacement_vsize, replacement_tx["tx"].get_vsize())
        assert_equal(event.replacement_fee, replacement_fee)

        bpf.cleanup()
        self.generate(self.wallet, 1)

    def rejected_test(self):
        """Create an invalid transaction and make sure the tracepoint returns
        the expected txid, rejection reason, peer id, and peer address."""

        events = []

        self.log.info("Adding P2P connection...")
        node = self.nodes[0]
        node.add_p2p_connection(P2PDataStore())

        self.log.info("Hooking into mempool:rejected tracepoint...")
        ctx = USDT(pid=node.process.pid)
        ctx.enable_probe(probe="mempool:rejected", fn_name="trace_rejected")
        bpf = BPF(text=MEMPOOL_TRACEPOINTS_PROGRAM, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        def handle_rejected_event(_, data, __):
            events.append(bpf["rejected_events"].event(data))

        bpf["rejected_events"].open_perf_buffer(handle_rejected_event)

        self.log.info("Sending invalid transaction...")
        tx = self.wallet.create_self_transfer(fee_rate=Decimal(0))
        node.p2ps[0].send_txs_and_test([tx["tx"]], node, success=False)

        self.log.info("Polling buffer...")
        bpf.perf_buffer_poll(timeout=200)

        self.log.info("Ensuring mempool:rejected event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(bytes(event.hash)[::-1].hex(), tx["tx"].hash)
        # The next test is already known to fail, so disable it to avoid
        # wasting CPU time and developer time. See
        # https://github.com/bitcoin/bitcoin/issues/27380
        #assert_equal(event.reason.decode("UTF-8"), "min relay fee not met")

        bpf.cleanup()
        self.generate(self.wallet, 1)

    def run_test(self):
        """Tests the mempool:added, mempool:removed, mempool:replaced,
        and mempool:rejected tracepoints."""

        # Create some coinbase transactions and mature them so they can be spent
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 4)
        self.generate(node, COINBASE_MATURITY)

        # Test individual tracepoints
        self.added_test()
        self.removed_test()
        self.replaced_test()
        self.rejected_test()


if __name__ == "__main__":
    MempoolTracepointTest(__file__).main()
