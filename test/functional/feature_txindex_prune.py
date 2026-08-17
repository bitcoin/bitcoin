#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test txindex in prune mode.

txindex can run with -prune. Lookups of transactions whose
blocks have been pruned fail with an error naming the block.
utxoupdatepsbt skips previous transactions that are only in pruned blocks.
"""

from http.client import HTTPConnection
from json import loads
from urllib.parse import urlparse

from test_framework.test_framework import BitcoinTestFramework
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
            ["-fastprune", "-prune=1", "-txindex", "-rest"],
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
        pruned = self.nodes[0]
        full = self.nodes[1]

        sync_txindex(self, pruned)
        wallet = MiniWallet(pruned)
        tx = wallet.send_to(from_node=pruned, scriptPubKey=getnewdestination("legacy")[1], amount=100_000)
        blockhash = self.generate(pruned, 1)[0]
        sync_txindex(self, pruned)
        txid = tx["txid"]
        parent_txid = pruned.decoderawtransaction(tx["hex"])["vin"][0]["txid"]
        parent_blockhash = pruned.getrawtransaction(parent_txid, 1)["blockhash"]

        self.log.info("Mine enough blocks to prune the transaction's block")
        self.generate(pruned, 400)
        sync_txindex(self, pruned)
        pruned.pruneblockchain(300)

        self.log.info("getrawtransaction, gettxoutproof, and REST /tx name the pruned block")
        msg = f"Transaction may be in pruned block {blockhash}"
        assert_raises_rpc_error(-1, msg, pruned.getrawtransaction, txid)
        assert msg in self.rest_tx(pruned, txid, status=404)
        # The tx's output is unspent, so gettxoutproof finds its block through
        # the UTXO set and fails with the pre-existing pruned error.
        assert_raises_rpc_error(-1, "Block not available (pruned data)", pruned.gettxoutproof, [txid])
        # The parent's output is spent, so gettxoutproof needs the txindex.
        assert_raises_rpc_error(-1, f"Transaction may be in pruned block {parent_blockhash}", pruned.gettxoutproof, [parent_txid])

        self.log.info("utxoupdatepsbt skips previous transactions that are only in pruned blocks")
        dest = getnewdestination()[2]
        psbt = pruned.createpsbt([{"txid": txid, "vout": tx["sent_vout"]}], [{dest: 0.0009}])
        updated = pruned.utxoupdatepsbt(psbt)
        assert "non_witness_utxo" not in pruned.decodepsbt(updated)["inputs"][0]

        self.log.info("Fetch the pruned block and retry")
        pruned.getblockfrompeer(blockhash, pruned.getpeerinfo()[0]["id"])

        def tx_available():
            try:
                return pruned.getrawtransaction(txid) == tx["hex"]
            except JSONRPCException:
                return False
        self.wait_until(tx_available, timeout=5)

        tx_verbose = pruned.getrawtransaction(txid, 2)
        assert "fee" not in tx_verbose
        assert "prevout" not in tx_verbose["vin"][0]
        assert_equal(self.rest_tx(pruned, txid)["txid"], txid)
        assert_equal(pruned.verifytxoutproof(pruned.gettxoutproof([txid])), [txid])
        updated = pruned.utxoupdatepsbt(psbt)
        assert "non_witness_utxo" in pruned.decodepsbt(updated)["inputs"][0]

        self.log.info("Enabling prune on an existing non-legacy txindex is allowed")
        self.restart_node(1, extra_args=["-fastprune", "-prune=1", "-txindex"])
        sync_txindex(self, full)
        assert_equal(full.getrawtransaction(txid), tx["hex"])


if __name__ == '__main__':
    TxIndexPruneTest(__file__).main()
