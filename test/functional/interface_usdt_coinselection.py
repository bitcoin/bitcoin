#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the coin_selection:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-coin_selection
"""

# Test will be skipped if we don't have bcc installed
try:
    from bcc import BPF, USDT # type: ignore[import]
except ImportError:
    pass
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)

coinselection_tracepoints_program = """
#include <uapi/linux/ptrace.h>

#define WALLET_NAME_LENGTH 16
#define ALGO_NAME_LENGTH 16

struct event_data
{
    u8 type;
    char wallet_name[WALLET_NAME_LENGTH];

    // selected coins event
    char algo[ALGO_NAME_LENGTH];
    s64 target;
    s64 waste;
    s64 selected_value;

    // create tx event
    bool success;
    s64 fee;
    s32 change_pos;

    // aps create tx event
    bool use_aps;
};

BPF_QUEUE(coin_selection_events, struct event_data, 1024);

int trace_selected_coins(struct pt_regs *ctx) {
    struct event_data data;
    __builtin_memset(&data, 0, sizeof(data));
    data.type = 1;
    bpf_usdt_readarg_p(1, ctx, &data.wallet_name, WALLET_NAME_LENGTH);
    bpf_usdt_readarg_p(2, ctx, &data.algo, ALGO_NAME_LENGTH);
    bpf_usdt_readarg(3, ctx, &data.target);
    bpf_usdt_readarg(4, ctx, &data.waste);
    bpf_usdt_readarg(5, ctx, &data.selected_value);
    coin_selection_events.push(&data, 0);
    return 0;
}

int trace_normal_create_tx(struct pt_regs *ctx) {
    struct event_data data;
    __builtin_memset(&data, 0, sizeof(data));
    data.type = 2;
    bpf_usdt_readarg_p(1, ctx, &data.wallet_name, WALLET_NAME_LENGTH);
    bpf_usdt_readarg(2, ctx, &data.success);
    bpf_usdt_readarg(3, ctx, &data.fee);
    bpf_usdt_readarg(4, ctx, &data.change_pos);
    coin_selection_events.push(&data, 0);
    return 0;
}

int trace_attempt_aps(struct pt_regs *ctx) {
    struct event_data data;
    __builtin_memset(&data, 0, sizeof(data));
    data.type = 3;
    bpf_usdt_readarg_p(1, ctx, &data.wallet_name, WALLET_NAME_LENGTH);
    coin_selection_events.push(&data, 0);
    return 0;
}

int trace_aps_create_tx(struct pt_regs *ctx) {
    struct event_data data;
    __builtin_memset(&data, 0, sizeof(data));
    data.type = 4;
    bpf_usdt_readarg_p(1, ctx, &data.wallet_name, WALLET_NAME_LENGTH);
    bpf_usdt_readarg(2, ctx, &data.use_aps);
    bpf_usdt_readarg(3, ctx, &data.success);
    bpf_usdt_readarg(4, ctx, &data.fee);
    bpf_usdt_readarg(5, ctx, &data.change_pos);
    coin_selection_events.push(&data, 0);
    return 0;
}
"""


class CoinSelectionTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_python_bcc()
        self.skip_if_no_bpf_permissions()
        self.skip_if_no_wallet()

    def get_tracepoints(self, expected_types):
        events = []
        try:
            for i in range(0, len(expected_types) + 1):
                event = self.bpf["coin_selection_events"].pop()
                assert_equal(event.wallet_name.decode(), self.default_wallet_name)
                assert_equal(event.type, expected_types[i])
                events.append(event)
            else:
                # If the loop exits successfully instead of throwing a KeyError, then we have had
                # more events than expected. There should be no more than len(expected_types) events.
                assert False
        except KeyError:
            assert_equal(len(events), len(expected_types))
            return events


    def determine_selection_from_usdt(self, events):
        success = None
        use_aps = None
        algo = None
        waste = None
        change_pos = None

        is_aps = False
        sc_events = []
        for event in events:
            if event.type == 1:
                if not is_aps:
                    algo = event.algo.decode()
                    waste = event.waste
                sc_events.append(event)
            elif event.type == 2:
                success = event.success
                if not is_aps:
                    change_pos = event.change_pos
            elif event.type == 3:
                is_aps = True
            elif event.type == 4:
                assert is_aps
                if event.use_aps:
                    use_aps = True
                    assert_equal(len(sc_events), 2)
                    algo = sc_events[1].algo.decode()
                    waste = sc_events[1].waste
                    change_pos = event.change_pos
        return success, use_aps, algo, waste, change_pos

    def run_test(self):
        self.log.info("hook into the coin_selection tracepoints")
        ctx = USDT(pid=self.nodes[0].process.pid)
        ctx.enable_probe(probe="coin_selection:selected_coins", fn_name="trace_selected_coins")
        ctx.enable_probe(probe="coin_selection:normal_create_tx_internal", fn_name="trace_normal_create_tx")
        ctx.enable_probe(probe="coin_selection:attempting_aps_create_tx", fn_name="trace_attempt_aps")
        ctx.enable_probe(probe="coin_selection:aps_create_tx_internal", fn_name="trace_aps_create_tx")
        self.bpf = BPF(text=coinselection_tracepoints_program, usdt_contexts=[ctx], debug=0, cflags=["-Wno-error=implicit-function-declaration"])

        self.log.info("Prepare wallets")
        self.generate(self.nodes[0], 101)
        wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.log.info("Sending a transaction should result in all tracepoints")
        # We should have 5 tracepoints in the order:
        # 1. selected_coins (type 1)
        # 2. normal_create_tx_internal (type 2)
        # 3. attempting_aps_create_tx (type 3)
        # 4. selected_coins (type 1)
        # 5. aps_create_tx_internal (type 4)
        wallet.sendtoaddress(wallet.getnewaddress(), 10)
        events = self.get_tracepoints([1, 2, 3, 1, 4])
        success, use_aps, algo, waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_greater_than(change_pos, -1)

        self.log.info("Failing to fund results in 1 tracepoint")
        # We should have 1 tracepoints in the order
        # 1. normal_create_tx_internal (type 2)
        assert_raises_rpc_error(-6, "Insufficient funds", wallet.sendtoaddress, wallet.getnewaddress(), 102 * 50)
        events = self.get_tracepoints([2])
        success, use_aps, algo, waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, False)

        self.log.info("Explicitly enabling APS results in 2 tracepoints")
        # We should have 2 tracepoints in the order
        # 1. selected_coins (type 1)
        # 2. normal_create_tx_internal (type 2)
        wallet.setwalletflag("avoid_reuse")
        wallet.sendtoaddress(address=wallet.getnewaddress(), amount=10, avoid_reuse=True)
        events = self.get_tracepoints([1, 2])
        success, use_aps, algo, waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_equal(use_aps, None)

        self.log.info("Change position is -1 if no change is created with APS when APS was initially not used")
        # We should have 2 tracepoints in the order:
        # 1. selected_coins (type 1)
        # 2. normal_create_tx_internal (type 2)
        # 3. attempting_aps_create_tx (type 3)
        # 4. selected_coins (type 1)
        # 5. aps_create_tx_internal (type 4)
        wallet.sendtoaddress(address=wallet.getnewaddress(), amount=wallet.getbalance(), subtractfeefromamount=True, avoid_reuse=False)
        events = self.get_tracepoints([1, 2, 3, 1, 4])
        success, use_aps, algo, waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_equal(change_pos, -1)

        self.log.info("Change position is -1 if no change is created normally and APS is not used")
        # We should have 2 tracepoints in the order:
        # 1. selected_coins (type 1)
        # 2. normal_create_tx_internal (type 2)
        wallet.sendtoaddress(address=wallet.getnewaddress(), amount=wallet.getbalance(), subtractfeefromamount=True)
        events = self.get_tracepoints([1, 2])
        success, use_aps, algo, waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_equal(change_pos, -1)

        self.bpf.cleanup()


if __name__ == '__main__':
    CoinSelectionTracepointTest().main()
