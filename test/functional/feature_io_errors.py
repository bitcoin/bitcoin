#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test RPC errors and fatal txindex failures on unreadable block files."""

from contextlib import closing
import http.client
import subprocess
import urllib.parse

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


class IOErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-txindex", "-rest"]]
        self.uses_wallet = None

    def test_blk_dat_io_error(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        tx, tx2 = wallet.send_self_transfer_chain(from_node=node, chain_length=2)
        txid = tx["txid"]
        self.generate(node, 1)
        psbt = node.createpsbt(inputs=[{"txid": txid, "vout": 0}], outputs=[])
        self.restart_node(0)  # Flush the mined block to disk, so it survives the aborts

        blk_dat = node.blocks_path / "blk00000.dat"
        blk_dat_moved = node.blocks_path / "blk00000.dat.moved"
        blk_dat.rename(blk_dat_moved)

        self.log.info("Check tx RPCs throw on I/O errors")
        msg = "Can't read block from disk"
        assert_raises_rpc_error(-32603, msg, node.gettxoutproof, [tx2["txid"]])
        blk_dat_moved.rename(blk_dat)

        def read_rest_tx():
            url = urllib.parse.urlparse(node.url)
            with closing(http.client.HTTPConnection(url.hostname, url.port)) as conn:
                conn.request("GET", f"/rest/tx/{txid}.json")
                conn.getresponse()

        self.log.info("Check txindex block-file errors abort through RPC and REST")
        msg = "txindex: Failed to open block file"
        for read_tx in (
            lambda: node.gettxoutproof([txid]),
            lambda: node.getrawtransaction(txid),
            lambda: node.utxoupdatepsbt(psbt),
            read_rest_tx,
        ):
            if not node.running:
                self.start_node(0)
            blk_dat.rename(blk_dat_moved)
            with node.assert_debug_log([msg]):
                try:
                    read_tx()
                except (subprocess.CalledProcessError, http.client.CannotSendRequest, ConnectionError):
                    pass  # Aborting closes the connection
                node.wait_until_aborted(fatal_error=msg)
            blk_dat_moved.rename(blk_dat)

    def run_test(self):
        self.test_blk_dat_io_error()


if __name__ == "__main__":
    IOErrorTest(__file__).main()
