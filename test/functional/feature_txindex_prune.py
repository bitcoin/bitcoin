#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test txindex initialization, pruning, lookups, and missing-blocks rejection."""

from http.client import HTTPConnection
from json import loads
from urllib.parse import urlparse

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    sync_txindex,
    JSONRPCException,
)
from test_framework.wallet import MiniWallet, getnewdestination


class TxIndexPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-fastprune", "-prune=1", "-rest"],
            ["-txindex"],
        ]

    def rest_tx(self, node, txid, status=200):
        url = urlparse(node.url)
        conn = HTTPConnection(url.hostname, url.port)
        conn.request("GET", f"/rest/tx/{txid}.json")
        resp = conn.getresponse()
        body = resp.read().decode("utf-8")
        assert_equal(resp.status, status)
        if status == 200:
            return loads(body)
        return body

    def run_test(self):
        pruned, full = self.nodes
        pruned_txindex_args = self.extra_args[0] + ["-txindex"]
        self.generate(full, 400)
        pruned.pruneblockchain(300)
        first_indexed_height = pruned.getblockchaininfo()["pruneheight"]

        self.log.info("A fresh txindex on a pruned node indexes only the available blocks")
        self.restart_node(0, extra_args=pruned_txindex_args)
        sync_txindex(self, pruned)
        for height in (first_indexed_height, pruned.getblockcount()):
            blockhash = pruned.getblockhash(height)
            tx = pruned.getblock(blockhash, 2)["tx"][0]
            indexed_tx = pruned.getrawtransaction(tx["txid"], 1)
            assert_equal(indexed_tx["hex"], tx["hex"])
            assert_equal(indexed_tx["blockhash"], blockhash)
        assert_equal(pruned.getindexinfo("txindex")["txindex"]["first_block_height"], first_indexed_height)
        self.connect_nodes(0, 1)

        self.log.info("Unknown transactions return possible missing block error message in RPC and REST")
        unknown_txid = "01" * 32
        warning = "The transaction may be in an earlier block not covered by this index"
        detail = f"{warning}, which starts at height {first_indexed_height}"
        assert_raises_rpc_error(-5, detail, pruned.getrawtransaction, unknown_txid)
        assert_raises_rpc_error(-5, detail, pruned.gettxoutproof, [unknown_txid])
        assert detail in self.rest_tx(pruned, unknown_txid, status=404)

        def assert_no_history_warning(call, *args):
            try:
                call(*args)
            except JSONRPCException as error:
                assert warning not in error.error["message"]
            else:
                raise AssertionError("Expected transaction lookup failure")

        # Explicit-block and non-pruned misses must not include the possible missing block error message.
        tip = pruned.getbestblockhash()
        assert_no_history_warning(pruned.getrawtransaction, unknown_txid, 0, tip)
        assert_no_history_warning(pruned.gettxoutproof, [unknown_txid], tip)
        sync_txindex(self, full)
        assert_no_history_warning(full.getrawtransaction, unknown_txid)
        assert_no_history_warning(full.gettxoutproof, [unknown_txid])

        wallet = MiniWallet(pruned, tag_name="pruned")
        self.generate(wallet, 101)
        tx = wallet.send_to(from_node=pruned, scriptPubKey=getnewdestination("legacy")[1], amount=100_000)
        blockhash = self.generate(pruned, 1)[0]
        txid = tx["txid"]
        parent_txid = pruned.decoderawtransaction(tx["hex"])["vin"][0]["txid"]
        parent_blockhash = pruned.getrawtransaction(parent_txid, 1)["blockhash"]

        self.log.info("Mine enough blocks to prune the transaction's block")
        self.generate(full, 400)
        pruned.pruneblockchain(pruned.getblockcount())
        self.restart_node(0, extra_args=pruned_txindex_args)
        sync_txindex(self, pruned)
        self.connect_nodes(0, 1)

        self.log.info("getrawtransaction and REST /tx return the pruned block's hash")
        msg = f"Transaction may be in pruned block {blockhash}"
        assert_raises_rpc_error(-1, msg, pruned.getrawtransaction, txid)
        assert msg in self.rest_tx(pruned, txid, status=404)
        self.log.info("gettxoutproof returns the parent block's hash")
        # The tx output is unspent, so gettxoutproof looks at the utxo set and fails with the original error.
        assert_raises_rpc_error(-1, "Block not available (pruned data)", pruned.gettxoutproof, [txid])
        # The parent's tx output is spent, so the lookup goes through the pruned txindex and returns the block hash.
        assert_raises_rpc_error(-1, f"Transaction may be in pruned block {parent_blockhash}", pruned.gettxoutproof, [parent_txid])

        self.log.info("utxoupdatepsbt skips previous transactions in pruned blocks")
        psbt = pruned.createpsbt([{"txid": txid, "vout": tx["sent_vout"]}], [{getnewdestination()[2]: 0.0009}])
        updated = pruned.utxoupdatepsbt(psbt)
        assert "non_witness_utxo" not in pruned.decodepsbt(updated)["inputs"][0]

        self.log.info("Fetch the pruned block and retry RPC, REST, and PSBT lookups")
        pruned.getblockfrompeer(blockhash, pruned.getpeerinfo()[0]["id"])

        def tx_available():
            try:
                return pruned.getrawtransaction(txid) == tx["hex"]
            except JSONRPCException:
                return False
        self.wait_until(tx_available, timeout=5)
        tx_verbose = pruned.getrawtransaction(txid, 2)
        # verbose fields are missing because there's no undo fetched from peer
        assert "fee" not in tx_verbose
        assert "prevout" not in tx_verbose["vin"][0]
        assert_equal(self.rest_tx(pruned, txid)["txid"], txid)
        assert_equal(pruned.verifytxoutproof(pruned.gettxoutproof([txid])), [txid])
        updated = pruned.utxoupdatepsbt(psbt)
        assert "non_witness_utxo" in pruned.decodepsbt(updated)["inputs"][0]

        self.log.info("An existing txindex cannot skip blocks that were pruned while it was disabled")
        self.restart_node(0, extra_args=self.extra_args[0])
        self.generate(pruned, 600, sync_fun=self.no_op)
        pruned.pruneblockchain(pruned.getblockcount())
        self.stop_node(0)
        pruned.assert_start_raises_init_error(
            extra_args=pruned_txindex_args,
            expected_msg="txindex best block of the index goes beyond pruned data",
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info("Enabling prune on an existing non-legacy txindex is allowed")
        self.restart_node(1, extra_args=["-fastprune", "-prune=1", "-txindex"])
        sync_txindex(self, full)
        assert_equal(full.getrawtransaction(txid), tx["hex"])
        assert_equal(full.getindexinfo("txindex")["txindex"]["first_block_height"], 0)

if __name__ == '__main__':
    TxIndexPruneTest(__file__).main()
