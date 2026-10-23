#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""  Tests the mempool:* tracepoint API interface.
     See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-mempool
"""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import COIN, DEFAULT_MEMPOOL_EXPIRY_HOURS
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.usdt import USDT
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MempoolTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_usdt_tools()
        self.skip_if_running_under_valgrind()

    def start_nodes(self):
        self.trace = USDT(self.nodes[0], {
            "mempool:added": [("hash", "h"), ("vsize", "i"), ("fee", "i")],
            "mempool:removed": [
                ("hash", "h"), ("reason", "s"), ("vsize", "i"), ("fee", "i"), ("entry_time", "i"),
            ],
            "mempool:rejected": [("hash", "h"), ("reason", "s")],
            "mempool:replaced": [
                ("replaced_hash", "h"), ("replaced_vsize", "i"), ("replaced_fee", "i"),
                ("replaced_entry_time", "i"), ("replacement_hash", "h"),
                ("replacement_vsize", "i"), ("replacement_fee", "i"), ("replaced_by_transaction", "i"),
            ],
        })
        super().start_nodes()

    def added_test(self):
        """Add a transaction to the mempool and make sure the tracepoint returns
        the expected txid, vsize, and fee."""

        node = self.nodes[0]
        self.trace.clear()

        self.log.info("Sending transaction...")
        fee = Decimal(31200)
        tx = self.wallet.send_self_transfer(from_node=node, fee=fee / COIN)

        events = [event for event in self.trace.read() if event.probe == "mempool:added"]

        self.log.info("Cleaning up mempool...")
        self.generate(node, 1)

        self.log.info("Ensuring mempool:added event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(event.hash, tx["txid"])
        assert_equal(event.vsize, tx["tx"].get_vsize())
        assert_equal(event.fee, fee)

        self.generate(self.wallet, 1)

    def removed_test(self):
        """Expire a transaction from the mempool and make sure the tracepoint returns
        the expected txid, expiry reason, vsize, and fee."""

        node = self.nodes[0]
        self.trace.clear()

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

        events = [event for event in self.trace.read() if event.probe == "mempool:removed"]

        self.log.info("Ensuring mempool:removed event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(event.hash, txid)
        assert_equal(event.reason, "expiry")
        assert_equal(event.vsize, tx["tx"].get_vsize())
        assert_equal(event.fee, fee)
        assert_equal(event.entry_time, entry_time)

        self.generate(self.wallet, 1)

    def replaced_test(self):
        """Replace one and two transactions in the mempool and make sure the tracepoint
        returns the expected txids, vsizes, and fees."""

        node = self.nodes[0]
        self.trace.clear()

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

        events = [event for event in self.trace.read() if event.probe == "mempool:replaced"]

        self.log.info("Ensuring mempool:replaced event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(event.replaced_hash, original_tx["txid"])
        assert_equal(event.replaced_vsize, original_tx["tx"].get_vsize())
        assert_equal(event.replaced_fee, original_fee)
        assert_equal(event.replaced_entry_time, entry_time)
        assert_equal(event.replacement_hash, replacement_tx["txid"])
        assert_equal(event.replacement_vsize, replacement_tx["tx"].get_vsize())
        assert_equal(event.replacement_fee, replacement_fee)
        assert_equal(event.replaced_by_transaction, True)

        self.generate(self.wallet, 1)

    def rejected_test(self):
        """Create an invalid transaction and make sure the tracepoint returns
        the expected txid, rejection reason, peer id, and peer address."""

        self.log.info("Adding P2P connection...")
        node = self.nodes[0]
        node.add_p2p_connection(P2PDataStore())

        self.trace.clear()

        self.log.info("Sending invalid transaction...")
        tx = self.wallet.create_self_transfer(fee_rate=Decimal(0))
        node.p2ps[0].send_txs_and_test([tx["tx"]], node, success=False)

        events = [event for event in self.trace.read() if event.probe == "mempool:rejected"]

        self.log.info("Ensuring mempool:rejected event was handled successfully...")
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(event.hash, tx["tx"].txid_hex)
        assert_equal(event.reason, "min relay fee not met")

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
