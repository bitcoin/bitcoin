#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test txindex lookups in prune mode and fetching missing blocks."""

from http.client import HTTPConnection
from urllib.parse import urlparse

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    sync_txindex,
    JSONRPCException,
)
from test_framework.wallet import MiniWallet, getnewdestination


class TxIndexPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.supports_cli = False
        self.extra_args = [
            ["-fastprune", "-prune=1", "-txindex", "-rest"],
            [],
        ]

    def rest_tx(self, node, txid, status=200):
        url = urlparse(node.url)
        conn = HTTPConnection(url.hostname, url.port)
        conn.request("GET", f"/rest/tx/{txid}.hex")
        resp = conn.getresponse()
        body = resp.read().decode("utf-8")
        assert_equal(resp.status, status)
        assert_equal(resp.getheader("Content-Type"), "text/plain")
        return body.strip()

    def run_test(self):
        pruned, full = self.nodes

        self.log.info("A txindex can sync from genesis with pruning enabled")
        wallet = MiniWallet(full)
        self.generate(wallet, 101)
        tx = wallet.send_to(from_node=pruned, scriptPubKey=getnewdestination("legacy")[1], amount=100_000)
        blockhash = self.generate(pruned, 1)[0]
        txid = tx["txid"]
        parent_txid = pruned.decoderawtransaction(tx["hex"])["vin"][0]["txid"]
        parent_blockhash = pruned.getrawtransaction(parent_txid, 1)["blockhash"]

        self.log.info("Mine enough blocks to prune the transaction's block")
        self.generate(full, 600)
        assert_equal(pruned.getrawtransaction(txid), tx["hex"])
        pruned.pruneblockchain(pruned.getblockcount())
        self.restart_node(0)
        sync_txindex(self, pruned)
        self.connect_nodes(0, 1)

        self.log.info("RPC lookups return pruned block hashes through txindex, UTXO, and explicit-block paths")
        for rpc, rpc_args, expected_blockhash in (
            (pruned.getrawtransaction, [txid], blockhash),
            (pruned.gettxoutproof, [[txid]], blockhash),
            (pruned.gettxoutproof, [[txid], blockhash], blockhash),
            # The parent's tx output is spent, so the lookup goes through the pruned txindex and returns the block hash.
            (pruned.gettxoutproof, [[parent_txid]], parent_blockhash),
        ):
            try:
                rpc(*rpc_args)
                raise AssertionError("Expected pruned transaction lookup failure")
            except JSONRPCException as error:
                assert_equal(error.error["code"], -1)
                assert_equal(error.error["message"], f"Transaction may be in pruned block {expected_blockhash}.")
                assert_equal(error.error["data"]["pruned_block_hashes"], [expected_blockhash])

        self.log.info("REST /tx returns the pruned block's hash")
        assert_equal(self.rest_tx(pruned, txid, status=404), f"Transaction may be in pruned block {blockhash}.")

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
        self.wait_until(tx_available)
        tx_verbose = pruned.getrawtransaction(txid, 2)
        # verbose fields are missing because there's no undo fetched from peer
        assert "fee" not in tx_verbose
        assert "prevout" not in tx_verbose["vin"][0]
        assert_equal(self.rest_tx(pruned, txid), tx["hex"])
        assert_equal(pruned.verifytxoutproof(pruned.gettxoutproof([txid])), [txid])
        updated = pruned.utxoupdatepsbt(psbt)
        assert "non_witness_utxo" in pruned.decodepsbt(updated)["inputs"][0]

if __name__ == '__main__':
    TxIndexPruneTest(__file__).main()
