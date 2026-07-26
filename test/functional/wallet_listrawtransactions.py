#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listrawtransactions RPC."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class ListRawTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_invalid_parameters()
        self.test_one_entry_per_tx()
        self.test_coinbase_tx()
        self.test_consolidation_tx_visible()
        self.test_count_and_skip()

    def test_invalid_parameters(self):
        self.log.info("Test listrawtransactions RPC parameter validity")
        assert_raises_rpc_error(-8, "Negative count", self.nodes[0].listrawtransactions, -1)
        assert_raises_rpc_error(-8, "Negative from", self.nodes[0].listrawtransactions, 10, -1)

    def test_one_entry_per_tx(self):
        self.log.info("Test that each tx appears exactly once, unlike listtransactions which shows one entry per output")
        self.generate(self.nodes[0], 101)

        # Send-to-self to a wallet receiving address (not change). listtransactions
        # produces two entries for this txid — one "send", one "receive" — because
        # it works per-output. listrawtransactions must produce exactly one.
        addr = self.nodes[0].getnewaddress()
        txid = self.nodes[0].sendtoaddress(addr, 0.5)

        lt_matching = [tx for tx in self.nodes[0].listtransactions("*", 20) if tx["txid"] == txid]
        assert_equal(len(lt_matching), 2)
        assert_equal({tx["category"] for tx in lt_matching}, {"send", "receive"})

        lrt_matching = [tx for tx in self.nodes[0].listrawtransactions(20) if tx["txid"] == txid]
        assert_equal(len(lrt_matching), 1)

    def test_coinbase_tx(self):
        self.log.info("Test that coinbase transactions appear in listrawtransactions with generated=True and no category")
        node = self.nodes[0]

        # Block 1's coinbase is mature (101 blocks generated in test_one_entry_per_tx).
        coinbase_txid = node.getblock(node.getblockhash(1))["tx"][0]

        results = node.listrawtransactions(9999)
        matching = [tx for tx in results if tx["txid"] == coinbase_txid]

        # Appears exactly once — same as any other tx.
        assert_equal(len(matching), 1)
        entry = matching[0]

        # Coinbase is flagged via "generated", not via a "category" field —
        # listrawtransactions intentionally omits category assignment.
        assert_equal(entry["generated"], True)
        assert "category" not in entry

        # Wallet received the block reward (positive amount, no fee).
        assert entry["amount"] > 0
        assert "fee" not in entry

    def test_consolidation_tx_visible(self):
        self.log.info("Test that consolidation tx (all inputs/outputs owned by wallet) appears in listrawtransactions")

        # Fund wallet with two UTXOs at distinct fresh addresses so listunspent
        # can retrieve each one unambiguously.
        addr1 = self.nodes[0].getnewaddress()
        addr2 = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(addr1, 1.0)
        self.nodes[0].sendtoaddress(addr2, 1.0)
        self.generate(self.nodes[0], 1)

        utxo1 = self.nodes[0].listunspent(1, 9999, [addr1])[0]
        utxo2 = self.nodes[0].listunspent(1, 9999, [addr2])[0]

        # Build consolidation tx: two inputs → one output, all wallet-owned.
        # The output must go to a change-keychain address so that listtransactions
        # omits it (outputs on the receiving keychain would appear as "receive").
        inputs = [{"txid": utxo1["txid"], "vout": utxo1["vout"]},
                  {"txid": utxo2["txid"], "vout": utxo2["vout"]}]
        total = utxo1["amount"] + utxo2["amount"]
        fee = Decimal("0.0001")
        outputs = {self.nodes[0].getrawchangeaddress(): float(total - fee)}

        raw_hex = self.nodes[0].createrawtransaction(inputs, outputs)
        signed = self.nodes[0].signrawtransactionwithwallet(raw_hex)
        assert_equal(signed["complete"], True)
        consolidation_txid = self.nodes[0].sendrawtransaction(signed["hex"])
        self.generate(self.nodes[0], 1)

        # listtransactions omits it — no logical category (neither send to an
        # external address nor receive from outside).
        lt_txids = [tx["txid"] for tx in self.nodes[0].listtransactions("*", 100)]
        assert consolidation_txid not in lt_txids, \
            f"consolidation txid {consolidation_txid} unexpectedly appeared in listtransactions"

        # listrawtransactions shows it exactly once.
        lrt_results = self.nodes[0].listrawtransactions(100)
        matching = [tx for tx in lrt_results if tx["txid"] == consolidation_txid]
        assert_equal(len(matching), 1)

        # amount = 0: no external flow (all funds stayed in the wallet).
        # fee is negative: reflects the miner fee paid.
        entry = matching[0]
        assert_equal(entry["amount"], Decimal("0"))
        assert_equal(entry["fee"], -fee)

    def test_count_and_skip(self):
        self.log.info("Test count and skip")
        node = self.nodes[0]

        all_txs = node.listrawtransactions(9999)
        total = len(all_txs)
        assert total >= 3, f"Need at least 3 txs for pagination test, got {total}"

        # count=2 returns the 2 newest txs ordered oldest-first within the result.
        page = node.listrawtransactions(2)
        assert_equal(len(page), 2)
        assert_equal(page[0]["txid"], all_txs[-2]["txid"])
        assert_equal(page[1]["txid"], all_txs[-1]["txid"])

        # skip=1 skips the newest tx; the next 2 are returned.
        page_skipped = node.listrawtransactions(2, 1)
        assert_equal(len(page_skipped), 2)
        assert_equal(page_skipped[0]["txid"], all_txs[-3]["txid"])
        assert_equal(page_skipped[1]["txid"], all_txs[-2]["txid"])

        # skip beyond total returns empty.
        assert_equal(len(node.listrawtransactions(10, total + 10)), 0)


if __name__ == "__main__":
    ListRawTransactionsTest(__file__).main()
