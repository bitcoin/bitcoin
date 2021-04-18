#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet import RPCs.

Test rescan behavior of importaddress, importpubkey, importprivkey, and
importmulti RPCs with different types of keys and rescan options.

In the first part of the test, node 0 creates an address for each type of
import RPC call and sends WCN to it. Then other nodes import the addresses,
and the test makes listtransactions and getbalance calls to confirm that the
importing node either did or did not execute rescans picking up the send
transactions.

In the second part of the test, node 0 sends more WCN to each address, and the
test makes more listtransactions and getbalance calls to confirm that the
importing nodes pick up the new transactions regardless of whether rescans
happened previously.
"""

from test_framework.test_framework import WidecoinTestFramework
from test_framework.address import AddressType
from test_framework.util import (
    assert_equal,
    set_node_times,
)

import collections
from decimal import Decimal
import enum
import itertools
import random

Call = enum.Enum("Call", "single multiaddress multiscript")
Data = enum.Enum("Data", "address pub priv")
Rescan = enum.Enum("Rescan", "no yes late_timestamp")


class Variant(collections.namedtuple("Variant", "call data address_type rescan prune")):
    """Helper for importing one key and verifying scanned transactions."""
    def do_import(self, timestamp):
        """Call one key import RPC."""
        rescan = self.rescan == Rescan.yes

        assert_equal(self.address["solvable"], True)
        assert_equal(self.address["isscript"], self.address_type == AddressType.p2sh_segwit)
        assert_equal(self.address["iswitness"], self.address_type == AddressType.bech32)
        if self.address["isscript"]:
            assert_equal(self.address["embedded"]["isscript"], False)
            assert_equal(self.address["embedded"]["iswitness"], True)

        if self.call == Call.single:
            if self.data == Data.address:
                response = self.node.importaddress(address=self.address["address"], label=self.label, rescan=rescan)
            elif self.data == Data.pub:
                response = self.node.importpubkey(pubkey=self.address["pubkey"], label=self.label, rescan=rescan)
            elif self.data == Data.priv:
                response = self.node.importprivkey(privkey=self.key, label=self.label, rescan=rescan)
            assert_equal(response, None)

        elif self.call in (Call.multiaddress, Call.multiscript):
            request = {
                "scriptPubKey": {
                    "address": self.address["address"]
                } if self.call == Call.multiaddress else self.address["scriptPubKey"],
                "timestamp": timestamp + TIMESTAMP_WINDOW + (1 if self.rescan == Rescan.late_timestamp else 0),
                "pubkeys": [self.address["pubkey"]] if self.data == Data.pub else [],
                "keys": [self.key] if self.data == Data.priv else [],
                "label": self.label,
                "watchonly": self.data != Data.priv
            }
            if self.address_type == AddressType.p2sh_segwit and self.data != Data.address:
                # We need solving data when providing a pubkey or privkey as data
                request.update({"redeemscript": self.address['embedded']['scriptPubKey']})
            response = self.node.importmulti(
                requests=[request],
                options={"rescan": self.rescan in (Rescan.yes, Rescan.late_timestamp)},
            )
            assert_equal(response, [{"success": True}])

    def check(self, txid=None, amount=None, confirmation_height=None):
        """Verify that listtransactions/listreceivedbyaddress return expected values."""

        txs = self.node.listtransactions(label=self.label, count=10000, include_watchonly=True)
        current_height = self.node.getblockcount()
        assert_equal(len(txs), self.expected_txs)

        addresses = self.node.listreceivedbyaddress(minconf=0, include_watchonly=True, address_filter=self.address['address'])
        if self.expected_txs:
            assert_equal(len(addresses[0]["txids"]), self.expected_txs)

        if txid is not None:
            tx, = [tx for tx in txs if tx["txid"] == txid]
            assert_equal(tx["label"], self.label)
            assert_equal(tx["address"], self.address["address"])
            assert_equal(tx["amount"], amount)
            assert_equal(tx["category"], "receive")
            assert_equal(tx["label"], self.label)
            assert_equal(tx["txid"], txid)
            assert_equal(tx["confirmations"], 1 + current_height - confirmation_height)
            assert_equal("trusted" not in tx, True)

            address, = [ad for ad in addresses if txid in ad["txids"]]
            assert_equal(address["address"], self.address["address"])
            assert_equal(address["amount"], self.expected_balance)
            assert_equal(address["confirmations"], 1 + current_height - confirmation_height)
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
IMPORT_VARIANTS = [Variant(*variants) for variants in itertools.product(Call, Data, AddressType, Rescan, (False, True))]

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

AMOUNT_DUST = 0.00000546


def get_rand_amount():
    r = random.uniform(AMOUNT_DUST, 1)
    return Decimal(str(round(r, 8)))


class ImportRescanTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2 + len(IMPORT_NODES)
        self.supports_cli = False
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.extra_args = [[] for _ in range(self.num_nodes)]
        for i, import_node in enumerate(IMPORT_NODES, 2):
            if import_node.prune:
                self.extra_args[i] += ["-prune=1"]

        self.add_nodes(self.num_nodes, extra_args=self.extra_args)

        # Import keys with pruning disabled
        self.start_nodes(extra_args=[[]] * self.num_nodes)
        self.import_deterministic_coinbase_privkeys()
        self.stop_nodes()

        self.start_nodes()
        for i in range(1, self.num_nodes):
            self.connect_nodes(i, 0)

    def run_test(self):
        # Create one transaction on node 0 with a unique amount for
        # each possible type of wallet import RPC.
        for i, variant in enumerate(IMPORT_VARIANTS):
            variant.label = "label {} {}".format(i, variant)
            variant.address = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress(
                label=variant.label,
                address_type=variant.address_type.value,
            ))
            variant.key = self.nodes[1].dumpprivkey(variant.address["address"])
            variant.initial_amount = get_rand_amount()
            variant.initial_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.initial_amount)
            self.nodes[0].generate(1)  # Generate one block for each send
            variant.confirmation_height = self.nodes[0].getblockcount()
            variant.timestamp = self.nodes[0].getblockheader(self.nodes[0].getbestblockhash())["time"]
        self.sync_all() # Conclude sync before calling setmocktime to avoid timeouts

        # Generate a block further in the future (past the rescan window).
        assert_equal(self.nodes[0].getrawmempool(), [])
        set_node_times(
            self.nodes,
            self.nodes[0].getblockheader(self.nodes[0].getbestblockhash())["time"] + TIMESTAMP_WINDOW + 1,
        )
        self.nodes[0].generate(1)
        self.sync_all()

        # For each variation of wallet key import, invoke the import RPC and
        # check the results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            self.log.info('Run import for variant {}'.format(variant))
            expect_rescan = variant.rescan == Rescan.yes
            variant.node = self.nodes[2 + IMPORT_NODES.index(ImportNode(variant.prune, expect_rescan))]
            variant.do_import(variant.timestamp)
            if expect_rescan:
                variant.expected_balance = variant.initial_amount
                variant.expected_txs = 1
                variant.check(variant.initial_txid, variant.initial_amount, variant.confirmation_height)
            else:
                variant.expected_balance = 0
                variant.expected_txs = 0
                variant.check()

        # Create new transactions sending to each address.
        for i, variant in enumerate(IMPORT_VARIANTS):
            variant.sent_amount = get_rand_amount()
            variant.sent_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.sent_amount)
            self.nodes[0].generate(1)  # Generate one block for each send
            variant.confirmation_height = self.nodes[0].getblockcount()

        assert_equal(self.nodes[0].getrawmempool(), [])
        self.sync_all()

        # Check the latest results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            self.log.info('Run check for variant {}'.format(variant))
            variant.expected_balance += variant.sent_amount
            variant.expected_txs += 1
            variant.check(variant.sent_txid, variant.sent_amount, variant.confirmation_height)


if __name__ == "__main__":
    ImportRescanTest().main()
