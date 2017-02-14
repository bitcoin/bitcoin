#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test rescan behavior of importaddress, importpubkey, importprivkey, and
importmulti RPCs with different types of keys and rescan options.

Test uses three connected nodes.

In the first part of the test, node 0 creates an address for each type of
import RPC call and sends BTC to it. Then nodes 1 and 2 import the addresses,
and the test makes listtransactions and getbalance calls to confirm that the
importing node either did or did not execute rescans picking up the send
transactions.

In the second part of the test, node 0 sends more BTC to each address, and the
test makes more listtransactions and getbalance calls to confirm that the
importing nodes pick up the new transactions regardless of whether rescans
happened previously.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (start_nodes, connect_nodes, sync_blocks, assert_equal, set_node_times)
from decimal import Decimal

import collections
import enum
import itertools

Call = enum.Enum("Call", "single multi")
Data = enum.Enum("Data", "address pub priv")
Rescan = enum.Enum("Rescan", "no yes late_timestamp")


class Variant(collections.namedtuple("Variant", "call data rescan")):
    """Helper for importing one key and verifying scanned transactions."""

    def do_import(self, timestamp):
        """Call one key import RPC."""

        if self.call == Call.single:
            if self.data == Data.address:
                response = self.node.importaddress(self.address["address"], self.label, self.rescan == Rescan.yes)
            elif self.data == Data.pub:
                response = self.node.importpubkey(self.address["pubkey"], self.label, self.rescan == Rescan.yes)
            elif self.data == Data.priv:
                response = self.node.importprivkey(self.key, self.label, self.rescan == Rescan.yes)
            assert_equal(response, None)
        elif self.call == Call.multi:
            response = self.node.importmulti([{
                "scriptPubKey": {
                    "address": self.address["address"]
                },
                "timestamp": timestamp + (1 if self.rescan == Rescan.late_timestamp else 0),
                "pubkeys": [self.address["pubkey"]] if self.data == Data.pub else [],
                "keys": [self.key] if self.data == Data.priv else [],
                "label": self.label,
                "watchonly": self.data != Data.priv
            }], {"rescan": self.rescan in (Rescan.yes, Rescan.late_timestamp)})
            assert_equal(response, [{"success": True}])

    def check(self, txid=None, amount=None, confirmations=None):
        """Verify that getbalance/listtransactions return expected values."""

        balance = self.node.getbalance(self.label, 0, True)
        assert_equal(balance, self.expected_balance)

        txs = self.node.listtransactions(self.label, 10000, 0, True)
        assert_equal(len(txs), self.expected_txs)

        if txid is not None:
            tx, = [tx for tx in txs if tx["txid"] == txid]
            assert_equal(tx["account"], self.label)
            assert_equal(tx["address"], self.address["address"])
            assert_equal(tx["amount"], amount)
            assert_equal(tx["category"], "receive")
            assert_equal(tx["label"], self.label)
            assert_equal(tx["txid"], txid)
            assert_equal(tx["confirmations"], confirmations)
            assert_equal("trusted" not in tx, True)
            if self.data != Data.priv:
                assert_equal(tx["involvesWatchonly"], True)
            else:
                assert_equal("involvesWatchonly" not in tx, True)


# List of Variants for each way a key or address could be imported.
IMPORT_VARIANTS = [Variant(*variants) for variants in itertools.product(Call, Data, Rescan)]


class ImportRescanTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 3

    def setup_network(self):
        extra_args = [["-debug=1"] for _ in range(self.num_nodes)]
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, extra_args)
        for i in range(1, self.num_nodes):
            connect_nodes(self.nodes[i], 0)

    def run_test(self):
        # Create one transaction on node 0 with a unique amount and label for
        # each possible type of wallet import RPC.
        for i, variant in enumerate(IMPORT_VARIANTS):
            variant.label = "label {} {}".format(i, variant)
            variant.address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress(variant.label))
            variant.key = self.nodes[0].dumpprivkey(variant.address["address"])
            variant.initial_amount = 25 - (i + 1) / 4.0
            variant.initial_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.initial_amount)

        # Generate a block containing the initial transactions, then another
        # block further in the future (past the rescan window).
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        timestamp = self.nodes[0].getblockheader(self.nodes[0].getbestblockhash())["time"]
        set_node_times(self.nodes, timestamp + 1)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # For each variation of wallet key import, invoke the import RPC and
        # check the results from getbalance and listtransactions. Import to
        # node 1 if rescanning is expected, and to node 2 if rescanning is not
        # expected. Node 2 is reserved for imports that do not cause rescans,
        # so later import calls don't inadvertently cause the wallet to pick up
        # transactions from earlier import calls where a rescan was not
        # expected (this would make it complicated to figure out expected
        # balances in the second part of the test.)
        for variant in IMPORT_VARIANTS:
            variant.node = self.nodes[1 if variant.rescan == Rescan.yes else 2]
            variant.do_import(timestamp)
            if variant.rescan == Rescan.yes:
                variant.expected_balance = variant.initial_amount
                variant.expected_txs = 1
                variant.check(variant.initial_txid, variant.initial_amount, 2)
            else:
                variant.expected_balance = 0
                variant.expected_txs = 0
                variant.check()

        # Create new transactions sending to each address.
        fee = self.nodes[0].getnetworkinfo()["relayfee"]
        for i, variant in enumerate(IMPORT_VARIANTS):
            variant.sent_amount = 25 - (2 * i + 1) / 8.0
            variant.sent_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.sent_amount)

        # Generate a block containing the new transactions.
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        sync_blocks(self.nodes)

        # Check the latest results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            variant.expected_balance += variant.sent_amount
            variant.expected_txs += 1
            variant.check(variant.sent_txid, variant.sent_amount, 1)


if __name__ == "__main__":
    ImportRescanTest().main()
