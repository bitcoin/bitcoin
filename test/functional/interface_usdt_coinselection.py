#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the coin_selection:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-coin_selection
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.usdt import USDT
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class CoinSelectionTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_usdt_tools()
        self.skip_if_no_wallet()
        self.skip_if_running_under_valgrind()

    def start_nodes(self):
        self.trace = USDT(self.nodes[0], {
            "coin_selection:selected_coins": [
                ("wallet_name", "s"), ("algo", "s"), ("target", "i"), ("waste", "i"), ("selected_value", "i"),
            ],
            "coin_selection:normal_create_tx_internal": [
                ("wallet_name", "s"), ("success", "i"), ("fee", "i"), ("change_pos", "i"),
            ],
            "coin_selection:attempting_aps_create_tx": [("wallet_name", "s")],
            "coin_selection:aps_create_tx_internal": [
                ("wallet_name", "s"), ("use_aps", "i"), ("success", "i"), ("fee", "i"), ("change_pos", "i"),
            ],
        })
        super().start_nodes()

    def get_tracepoints(self, expected_probes):
        events = self.trace.read()
        assert_equal([event.probe for event in events], [f"coin_selection:{probe}" for probe in expected_probes])
        for event in events:
            assert_equal(event.wallet_name, self.default_wallet_name)
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
            if event.probe == "coin_selection:selected_coins":
                if not is_aps:
                    algo = event.algo
                    waste = event.waste
                sc_events.append(event)
            elif event.probe == "coin_selection:normal_create_tx_internal":
                success = event.success
                if not is_aps:
                    change_pos = event.change_pos
            elif event.probe == "coin_selection:attempting_aps_create_tx":
                is_aps = True
            elif event.probe == "coin_selection:aps_create_tx_internal":
                assert is_aps
                if event.use_aps:
                    use_aps = True
                    assert_equal(len(sc_events), 2)
                    algo = sc_events[1].algo
                    waste = sc_events[1].waste
                    change_pos = event.change_pos
        return success, use_aps, algo, waste, change_pos

    def run_test(self):
        self.log.info("Prepare wallets")
        self.generate(self.nodes[0], 101)
        wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.log.info("Sending a transaction should result in all tracepoints")
        # We should have 5 tracepoints in the order:
        # 1. selected_coins
        # 2. normal_create_tx_internal
        # 3. attempting_aps_create_tx
        # 4. selected_coins
        # 5. aps_create_tx_internal
        self.trace.clear()
        wallet.sendtoaddress(wallet.getnewaddress(), 10)
        events = self.get_tracepoints([
            "selected_coins", "normal_create_tx_internal", "attempting_aps_create_tx",
            "selected_coins", "aps_create_tx_internal",
        ])
        success, use_aps, _algo, _waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_greater_than(change_pos, -1)

        self.log.info("Failing to fund results in 1 tracepoint")
        # We should have 1 tracepoints in the order
        # 1. normal_create_tx_internal
        self.trace.clear()
        assert_raises_rpc_error(-6, "Insufficient funds", wallet.sendtoaddress, wallet.getnewaddress(), 102 * 50)
        events = self.get_tracepoints(["normal_create_tx_internal"])
        success, use_aps, _algo, _waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, False)

        self.log.info("Explicitly enabling APS results in 2 tracepoints")
        # We should have 2 tracepoints in the order
        # 1. selected_coins
        # 2. normal_create_tx_internal
        wallet.setwalletflag("avoid_reuse")
        self.trace.clear()
        wallet.sendtoaddress(address=wallet.getnewaddress(), amount=10, avoid_reuse=True)
        events = self.get_tracepoints(["selected_coins", "normal_create_tx_internal"])
        success, use_aps, _algo, _waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_equal(use_aps, None)

        self.log.info("Change position is -1 if no change is created with APS when APS was initially not used")
        # We should have 2 tracepoints in the order:
        # 1. selected_coins
        # 2. normal_create_tx_internal
        # 3. attempting_aps_create_tx
        # 4. selected_coins
        # 5. aps_create_tx_internal
        self.trace.clear()
        wallet.sendtoaddress(address=wallet.getnewaddress(), amount=wallet.getbalance(), subtractfeefromamount=True, avoid_reuse=False)
        events = self.get_tracepoints([
            "selected_coins", "normal_create_tx_internal", "attempting_aps_create_tx",
            "selected_coins", "aps_create_tx_internal",
        ])
        success, use_aps, _algo, _waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_equal(change_pos, -1)

        self.log.info("Change position is -1 if no change is created normally and APS is not used")
        # We should have 2 tracepoints in the order:
        # 1. selected_coins
        # 2. normal_create_tx_internal
        self.trace.clear()
        wallet.sendtoaddress(address=wallet.getnewaddress(), amount=wallet.getbalance(), subtractfeefromamount=True)
        events = self.get_tracepoints(["selected_coins", "normal_create_tx_internal"])
        success, use_aps, _algo, _waste, change_pos = self.determine_selection_from_usdt(events)
        assert_equal(success, True)
        assert_equal(change_pos, -1)


if __name__ == '__main__':
    CoinSelectionTracepointTest(__file__).main()
