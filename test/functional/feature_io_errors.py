#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Test handling of I/O errors."""


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class IOErrorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.uses_wallet = None

    def test_blk_dat_io_error(self):
        self.log.info("Test RPCs throw on I/O errors")

        node = self.nodes[0]
        self.restart_node(0, ["-txindex"])
        self.wait_until(lambda: node.getindexinfo()["txindex"]["synced"])
        wallet = MiniWallet(node)

        tx = wallet.send_self_transfer(from_node=node)
        txid = tx["txid"]
        tx2 = wallet.send_self_transfer(from_node=node, utxo_to_spend=tx["new_utxo"])
        block = self.generate(node, 1)[0]
        if self.is_wallet_compiled():
            psbt = node.createpsbt(inputs=[{"txid": txid, "vout": 0}], outputs=[])

        blk_dat = node.blocks_path / "blk00000.dat"
        blk_dat_moved = node.blocks_path / "blk00000.dat.moved"
        blk_dat.rename(blk_dat_moved)

        msg = "I/O error while opening block file via txindex"
        assert_raises_rpc_error(-32603, msg, node.gettxoutproof, [txid])
        assert_raises_rpc_error(-32603, msg, node.getrawtransaction, txid)
        if self.is_wallet_compiled():
            assert_raises_rpc_error(-32603, msg, node.utxoupdatepsbt, psbt)
        msg = "I/O error reading block data"
        assert_raises_rpc_error(-32603, msg, node.getrawtransaction, "a" * 64, blockhash=block)
        msg = "Can't read block from disk"
        assert_raises_rpc_error(-32603, msg, node.gettxoutproof, [tx2["txid"]])

        blk_dat_moved.rename(blk_dat)

    def run_test(self):
        self.test_blk_dat_io_error()


if __name__ == "__main__":
    IOErrorTest(__file__).main()
