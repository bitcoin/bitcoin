#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test handling of I/O errors."""

from contextlib import closing
import http.client
import urllib.parse

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


class IOErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.uses_wallet = None

    def test_blk_dat_io_error(self):
        node = self.nodes[0]
        self.restart_node(0, ["-txindex", "-rest"])
        self.wait_until(lambda: node.getindexinfo()["txindex"]["synced"])
        wallet = MiniWallet(node)

        tx = wallet.send_self_transfer(from_node=node)
        txid = tx["txid"]
        tx2 = wallet.send_self_transfer(from_node=node, utxo_to_spend=tx["new_utxo"])
        self.generate(node, 1)
        psbt = node.createpsbt(inputs=[{"txid": txid, "vout": 0}], outputs=[])

        blk_dat = node.blocks_path / "blk00000.dat"
        blk_dat_moved = node.blocks_path / "blk00000.dat.moved"
        blk_dat.rename(blk_dat_moved)

        self.log.info("Check tx RPCs throw on I/O errors")
        msg = "Can't read block from disk"
        assert_raises_rpc_error(-32603, msg, node.gettxoutproof, [tx2["txid"]])
        assert_raises_rpc_error(-5, "Transaction not yet in block", node.gettxoutproof, [txid])
        assert_raises_rpc_error(-5, "No such mempool or blockchain transaction", node.getrawtransaction, txid)
        node.utxoupdatepsbt(psbt)  # no exception raised on IO error

        self.log.info("Check REST /tx/ on I/O errors")
        url = urllib.parse.urlparse(node.url)
        with closing(http.client.HTTPConnection(url.hostname, url.port)) as conn:
            conn.request("GET", f"/rest/tx/{txid}.json")
            response = conn.getresponse()
            assert_equal(response.status, 404)
            assert_equal(response.read().decode("utf-8"), f"{txid} not found\r\n")

        blk_dat_moved.rename(blk_dat)

    def run_test(self):
        self.test_blk_dat_io_error()


if __name__ == "__main__":
    IOErrorTest(__file__).main()
