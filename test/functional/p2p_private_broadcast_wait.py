#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test private-broadcast RPC acceptance waits, timeouts, and shutdown."""

from concurrent.futures import ThreadPoolExecutor
from contextlib import ExitStack

from test_framework.messages import malleate_tx_to_invalid_witness, msg_tx
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class PrivateBroadcastWaitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.disable_autoconnect = False
        # We don't use the proxy in this test. This is just a dummy so the node will start with privatebroadcast.
        self.extra_args = [["-privatebroadcast", "-proxy=127.0.0.1:1"]]

    def run_test(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())
        wallet = MiniWallet(node)
        rpc = node.create_new_rpc_connection()

        with ThreadPoolExecutor(max_workers=1) as executor, ExitStack() as cleanup:
            cleanup.callback(node.stop_node)

            self.log.info("Timeout leaves the broadcast queued")
            tx = wallet.create_self_transfer()
            assert_raises_rpc_error(-25, "Timed out waiting for mempool acceptance", node.sendrawtransaction, tx["hex"], timeout=100)
            assert_equal([t["wtxid"] for t in node.getprivatebroadcastinfo()["transactions"]], [tx["wtxid"]])

            def submit(tx):
                # Wait for this RPC, even when a previous attempt is already queued.
                with node.assert_debug_log([f"txid={tx['txid']}, wtxid={tx['wtxid']}"], timeout=10):
                    future = executor.submit(rpc.sendrawtransaction, tx["hex"])
                return future

            self.log.info("Retrying with the default timeout waits for mempool acceptance")
            result = submit(tx)
            peer.send_and_ping(msg_tx(malleate_tx_to_invalid_witness(tx)))
            assert not result.done()
            peer.send_and_ping(msg_tx(tx["tx"]))
            assert_equal(result.result(timeout=10), tx["txid"])

            self.log.info("Shutdown interrupts a wait")
            result = submit(wallet.create_self_transfer())
            node.stop()
            assert_raises_rpc_error(-9, "Shutting down", result.result, timeout=10)
            node.wait_until_stopped()


if __name__ == "__main__":
    PrivateBroadcastWaitTest(__file__).main()
