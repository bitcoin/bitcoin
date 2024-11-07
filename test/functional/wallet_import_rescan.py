#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
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
from test_framework.address import (
    AddressType,
    ADDRESS_BCRT1_UNSPENDABLE,
)
from test_framework.messages import COIN
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
                rescan=self.rescan in (Rescan.yes, Rescan.late_timestamp),
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

            # If no confirmation height is given, the tx is still in the
            # mempool.
            confirmations = (1 + current_height - confirmation_height) if confirmation_height else 0
            assert_equal(tx["confirmations"], confirmations)
            if confirmations:
                assert "trusted" not in tx

            address, = [ad for ad in addresses if txid in ad["txids"]]
            assert_equal(address["address"], self.address["address"])
            assert_equal(address["amount"], self.amount_received)
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


def get_rand_amount(min_amount=AMOUNT_DUST):
    assert min_amount <= 1
    r = random.uniform(min_amount, 1)
    # note: min_amount can get rounded down here
    return Decimal(str(round(r, 8)))


class ImportRescanTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=False)

    def set_test_params(self):
        self.num_nodes = 2 + len(IMPORT_NODES)
        self.supports_cli = False
        self.rpc_timeout = 120
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

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
        last_variants = []
        for i, variant in enumerate(IMPORT_VARIANTS):
            if i % 10 == 0:
                blockhash = self.generate(self.nodes[0], 1)[0]
                conf_height = self.nodes[0].getblockcount()
                timestamp = self.nodes[0].getblockheader(blockhash)["time"]
                for var in last_variants:
                    var.confirmation_height = conf_height
                    var.timestamp = timestamp
                last_variants.clear()
            variant.label = "label {} {}".format(i, variant)
            variant.address = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress(
                label=variant.label,
                address_type=variant.address_type.value,
            ))
            variant.key = self.nodes[1].dumpprivkey(variant.address["address"])
            variant.initial_amount = get_rand_amount()
            variant.initial_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.initial_amount)
            last_variants.append(variant)

        blockhash = self.generate(self.nodes[0], 1)[0]
        conf_height = self.nodes[0].getblockcount()
        timestamp = self.nodes[0].getblockheader(blockhash)["time"]
        for var in last_variants:
            var.confirmation_height = conf_height
            var.timestamp = timestamp
        last_variants.clear()

        # Generate a block further in the future (past the rescan window).
        assert_equal(self.nodes[0].getrawmempool(), [])
        set_node_times(
            self.nodes,
            self.nodes[0].getblockheader(self.nodes[0].getbestblockhash())["time"] + TIMESTAMP_WINDOW + 1,
        )
        self.generate(self.nodes[0], 1)

        # For each variation of wallet key import, invoke the import RPC and
        # check the results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            self.log.info('Run import for variant {}'.format(variant))
            expect_rescan = variant.rescan == Rescan.yes
            variant.node = self.nodes[2 + IMPORT_NODES.index(ImportNode(variant.prune, expect_rescan))]
            variant.do_import(variant.timestamp)
            if expect_rescan:
                variant.amount_received = variant.initial_amount
                variant.expected_txs = 1
                variant.check(variant.initial_txid, variant.initial_amount, variant.confirmation_height)
            else:
                variant.amount_received = 0
                variant.expected_txs = 0
                variant.check()

        # Create new transactions sending to each address.
        for i, variant in enumerate(IMPORT_VARIANTS):
            if i % 10 == 0:
                blockhash = self.generate(self.nodes[0], 1)[0]
                conf_height = self.nodes[0].getblockcount() + 1
            variant.sent_amount = get_rand_amount()
            variant.sent_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.sent_amount)
            variant.confirmation_height = conf_height
        self.generate(self.nodes[0], 1)

        assert_equal(self.nodes[0].getrawmempool(), [])
        self.sync_all()

        # Check the latest results from getbalance and listtransactions.
        for variant in IMPORT_VARIANTS:
            self.log.info('Run check for variant {}'.format(variant))
            variant.amount_received += variant.sent_amount
            variant.expected_txs += 1
            variant.check(variant.sent_txid, variant.sent_amount, variant.confirmation_height)

        self.log.info('Test that the mempool is rescanned as well if the rescan parameter is set to true')

        # The late timestamp and pruned variants are not necessary when testing mempool rescan
        mempool_variants = [variant for variant in IMPORT_VARIANTS if variant.rescan != Rescan.late_timestamp and not variant.prune]
        # No further blocks are mined so the timestamp will stay the same
        timestamp = self.nodes[0].getblockheader(self.nodes[0].getbestblockhash())["time"]

        # Create one transaction on node 0 with a unique amount for
        # each possible type of wallet import RPC.
        for i, variant in enumerate(mempool_variants):
            variant.label = "mempool label {} {}".format(i, variant)
            variant.address = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress(
                label=variant.label,
                address_type=variant.address_type.value,
            ))
            variant.key = self.nodes[1].dumpprivkey(variant.address["address"])
            # Ensure output is large enough to pay for fees: conservatively assuming txsize of
            # 500 vbytes and feerate of 20 sats/vbytes
            variant.initial_amount = get_rand_amount(min_amount=((500 * 20 / COIN) + AMOUNT_DUST))
            variant.initial_txid = self.nodes[0].sendtoaddress(variant.address["address"], variant.initial_amount)
            variant.confirmation_height = 0
            variant.timestamp = timestamp

        # Mine a block so these parents are confirmed
        assert_equal(len(self.nodes[0].getrawmempool()), len(mempool_variants))
        self.sync_mempools()
        block_to_disconnect = self.generate(self.nodes[0], 1)[0]
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        # For each variant, create an unconfirmed child transaction from initial_txid, sending all
        # the funds to an unspendable address. Importantly, no change output is created so the
        # transaction can't be recognized using its outputs. The wallet rescan needs to know the
        # inputs of the transaction to detect it, so the parent must be processed before the child.
        # An equivalent test for descriptors exists in wallet_rescan_unconfirmed.py.
        unspent_txid_map = {txin["txid"] : txin for txin in self.nodes[1].listunspent()}
        for variant in mempool_variants:
            # Send full amount, subtracting fee from outputs, to ensure no change is created.
            child = self.nodes[1].send(
                add_to_wallet=False,
                inputs=[unspent_txid_map[variant.initial_txid]],
                outputs=[{ADDRESS_BCRT1_UNSPENDABLE : variant.initial_amount}],
                subtract_fee_from_outputs=[0]
            )
            variant.child_txid = child["txid"]
            variant.amount_received = 0
            self.nodes[0].sendrawtransaction(child["hex"])

        # Mempools should contain the child transactions for each variant.
        assert_equal(len(self.nodes[0].getrawmempool()), len(mempool_variants))
        self.sync_mempools()

        # Mock a reorg so the parent transactions are added back to the mempool
        for node in self.nodes:
            node.invalidateblock(block_to_disconnect)
            # Mempools should now contain the parent and child for each variant.
            assert_equal(len(node.getrawmempool()), 2 * len(mempool_variants))

        # For each variation of wallet key import, invoke the import RPC and
        # check the results from getbalance and listtransactions.
        for variant in mempool_variants:
            self.log.info('Run import for mempool variant {}'.format(variant))
            expect_rescan = variant.rescan == Rescan.yes
            variant.node = self.nodes[2 + IMPORT_NODES.index(ImportNode(variant.prune, expect_rescan))]
            variant.do_import(variant.timestamp)
            if expect_rescan:
                # Ensure both transactions were rescanned. This would raise a JSONRPCError if the
                # transactions were not identified as belonging to the wallet.
                assert_equal(variant.node.gettransaction(variant.initial_txid)['confirmations'], 0)
                assert_equal(variant.node.gettransaction(variant.child_txid)['confirmations'], 0)
                variant.amount_received = variant.initial_amount
                variant.expected_txs = 1
                variant.check(variant.initial_txid, variant.initial_amount, 0)
            else:
                variant.amount_received = 0
                variant.expected_txs = 0
                variant.check()


if __name__ == "__main__":
    ImportRescanTest(__file__).main()
