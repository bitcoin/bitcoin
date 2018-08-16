#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet import RPCs.

Test rescan behavior of importaddress, importpubkey, importprivkey, and
importmulti RPCs with different types of keys and rescan options.

In the first part of the test, node 0 creates an address for each type of
import RPC call and sends BTC to it. Then other nodes import the addresses,
and the test makes listtransactions and getbalance calls to confirm that the
importing node either did or did not execute rescans picking up the send
transactions.

In the second part of the test, node 0 sends more BTC to each address, and the
test makes more listtransactions and getbalance calls to confirm that the
importing nodes pick up the new transactions regardless of whether rescans
happened previously.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_raises_rpc_error, connect_nodes, sync_blocks, assert_equal, set_node_times)

import collections
import enum
import itertools

Call = enum.Enum("Call", "single multi")
Data = enum.Enum("Data", "address pub priv")
Rescan = enum.Enum("Rescan", "no yes late_timestamp")


class Variant(collections.namedtuple("Variant", "call data rescan prune")):
    """Helper for importing one key and verifying scanned transactions."""

    def try_rpc(self, func, *args, **kwargs):
        if self.expect_disabled:
            assert_raises_rpc_error(-4, "Rescan is disabled in pruned mode", func, *args, **kwargs)
        else:
            return func(*args, **kwargs)

    def do_import(self, timestamp):
        """Call one key import RPC."""
        rescan = self.rescan == Rescan.yes

        if self.call == Call.single:
            if self.data == Data.address:
                response = self.try_rpc(self.node.importaddress, address=self.address["address"], rescan=rescan)
            elif self.data == Data.pub:
                response = self.try_rpc(self.node.importpubkey, pubkey=self.address["pubkey"], rescan=rescan)
            elif self.data == Data.priv:
                response = self.try_rpc(self.node.importprivkey, privkey=self.key, rescan=rescan)
            assert_equal(response, None)

        elif self.call == Call.multi:
            response = self.node.importmulti([{
                "scriptPubKey": {
                    "address": self.address["address"]
                },
                "timestamp": timestamp + TIMESTAMP_WINDOW + (1 if self.rescan == Rescan.late_timestamp else 0),
                "pubkeys": [self.address["pubkey"]] if self.data == Data.pub else [],
                "keys": [self.key] if self.data == Data.priv else [],
                "watchonly": self.data != Data.priv
            }], {"rescan": self.rescan in (Rescan.yes, Rescan.late_timestamp)})
            assert_equal(response, [{"success": True}])

    def check(self, txid=None, amount=None, confirmations=None):
        """Verify that listreceivedbyaddress returns expected values."""

        addresses = self.node.listreceivedbyaddress(minconf=0, include_watchonly=True, address_filter=self.address['address'])
        if self.expected_txs:
            assert_equal(len(addresses[0]["txids"]), self.expected_txs)

        if txid is not None:
            address, = [ad for ad in addresses if txid in ad["txids"]]
            assert_equal(address["address"], self.address["address"])
            assert_equal(address["amount"], self.expected_balance)
            assert_equal(address["confirmations"], confirmations)
            # Verify the transaction is correctly marked watchonly depending on
            # whether the transaction pays to an imported public key or
            # imported private key. The test setup ensures that transaction
            # inputs will not be from watchonly keys (important because
            # involvesWatchonly will be true if either the transaction output
            # or inputs are watchonly).
            if self.data != Data.priv:
                assert_equal(address["involvesWatchonly"], True)
            else:
                assert_equal("involvesWatchonly" not in address, True)


# List of Variants for each way a key or address could be imported.
IMPORT_VARIANTS = [Variant(*variants) for variants in itertools.product(Call, Data, Rescan, (False, True))]

# List of nodes to import keys to. Half the nodes will have pruning disabled,
# half will have it enabled. Different nodes will be used for imports that are
# expected to cause rescans, and imports that are not expected to cause
# rescans, in order to prevent rescans during later imports picking up
# transactions associated with earlier imports. This makes it easier to keep
# track of expected balances and transactions.
ImportNode = collections.namedtuple("ImportNode", "prune rescan")
IMPORT_NODES = [ImportNode(*fields) for fields in itertools.product((False, True), repeat=2)]

# Rescans start at the earliest block up to 2 hours before the key timestamp.
TIMESTAMP_WINDOW = 2 * 60 * 60


class ImportRescanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2 + len(IMPORT_NODES)

    def setup_network(self):
        extra_args = [["-addresstype=legacy"] for _ in range(self.num_nodes)]
        for i, import_node in enumerate(IMPORT_NODES, 2):
            if import_node.prune:
                extra_args[i] += ["-prune=1"]

        self.add_nodes(self.num_nodes, extra_args=extra_args)
        self.start_nodes()
        for i in range(1, self.num_nodes):
            connect_nodes(self.nodes[i], 0)

    def run_test(self):
        # Create one transaction on node 0 with a unique amount for
        # each possible type of wallet import RPC.
        for i, variant in enumerate(IMPORT_VARIANTS):
            variant.address = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())
            variant.key = self.nodes[1].dumpprivkey(variant.address["address"])
            variant.initial_amount = 10 - (i + 1) / 4.0
            variant.initial_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.initial_amount)

        # Generate a block containing the initial transactions, then another
        # block further in the future (past the rescan window).
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        timestamp = self.nodes[0].getblockheader(self.nodes[0].getbestblockhash())["time"]
        set_node_times(self.nodes, timestamp + TIMESTAMP_WINDOW + 1)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # For each variation of wallet key import, invoke the import RPC and
        # check the results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            variant.expect_disabled = variant.rescan == Rescan.yes and variant.prune and variant.call == Call.single
            expect_rescan = variant.rescan == Rescan.yes and not variant.expect_disabled
            variant.node = self.nodes[2 + IMPORT_NODES.index(ImportNode(variant.prune, expect_rescan))]
            variant.do_import(timestamp)
            if expect_rescan:
                variant.expected_balance = variant.initial_amount
                variant.expected_txs = 1
                variant.check(variant.initial_txid, variant.initial_amount, 2)
            else:
                variant.expected_balance = 0
                variant.expected_txs = 0
                variant.check()

        # Create new transactions sending to each address.
        for i, variant in enumerate(IMPORT_VARIANTS):
            variant.sent_amount = 10 - (2 * i + 1) / 8.0
            variant.sent_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.sent_amount)

        # Generate a block containing the new transactions.
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        sync_blocks(self.nodes)

        # Check the latest results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            if not variant.expect_disabled:
                variant.expected_balance += variant.sent_amount
                variant.expected_txs += 1
                variant.check(variant.sent_txid, variant.sent_amount, 1)
            else:
                variant.check()

if __name__ == "__main__":
    ImportRescanTest().main()
