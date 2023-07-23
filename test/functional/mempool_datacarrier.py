#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test datacarrier functionality"""
from test_framework.messages import (
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import TestNode
from test_framework.util import (
    assert_raises_rpc_error,
    random_bytes,
)
from test_framework.wallet import MiniWallet

from typing import List

class DataCarrierTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            [],
            ["-datacarriersize=0"],
            ["-datacarriersize=100"]
        ]

    def test_null_data_transaction(self, node: TestNode, datas: List[bytes], success: bool) -> None:
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]

        for data in datas:
            tx.vout.append(CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN, data])))

        tx.vout[0].nValue -= tx.get_vsize()  # simply pay 1sat/vbyte fee

        tx_hex = tx.serialize().hex()

        if success:
            self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            assert tx.rehash() in node.getrawmempool(True), f'{tx_hex} not in mempool'
        else:
            assert_raises_rpc_error(-26, "scriptpubkey", self.wallet.sendrawtransaction, from_node=node, tx_hex=tx_hex)

    def test_bare_op_return(self, node: TestNode, n: int, success: bool) -> None:
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]

        for i in range(0, n):
            tx.vout.append(CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN])))
        tx.vout[0].nValue -= tx.get_vsize()  # simply pay 1sat/vbyte fee

        tx_hex = tx.serialize().hex()

        if success:
            self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            assert tx.rehash() in node.getrawmempool(True), f'{tx_hex} not in mempool'
        else:
            assert_raises_rpc_error(-26, "scriptpubkey", self.wallet.sendrawtransaction, from_node=node, tx_hex=tx_hex)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Pushdata opcode takes up 2 bytes
        exact_size_data = random_bytes(100 - 2)
        too_large_data = random_bytes(100 - 1)
        small_data = random_bytes(100 - 3)

        self.log.info("Testing null data transaction without -datacarriersize limit.")
        self.test_null_data_transaction(node=self.nodes[0], datas=[random_bytes(99800)], success=True)

        self.log.info("Testing multiple null data outputs without -datacarriersize limit.")
        self.test_null_data_transaction(node=self.nodes[0], datas=[random_bytes(100), random_bytes(100)], success=True)

        self.log.info("Testing a null data transaction with -datacarriersize=0.")
        self.test_null_data_transaction(node=self.nodes[1], datas=[b''], success=False)

        self.log.info("Testing a null data transaction with a size larger than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], datas=[too_large_data], success=False)

        self.log.info("Testing a null data transaction with a size exactly equal to accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], datas=[exact_size_data], success=True)

        self.log.info("Testing a null data transaction with a size smaller than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], datas=[small_data], success=True)

        self.log.info("Testing that a single and multiple bare OP_RETURN is always accepted")
        for n in (1, 2):
            for node in self.nodes:
                self.test_bare_op_return(node=node, n=n, success=True)

if __name__ == '__main__':
    DataCarrierTest().main()
