#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test datacarrier functionality"""
from test_framework.messages import (
    CTxOut,
    MAX_OP_RETURN_RELAY,
)
from test_framework.script import (
    CScript,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import TestNode
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet

from random import randbytes

# The historical maximum, now used to test coverage
CUSTOM_DATACARRIER_ARG = 83

class DataCarrierTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.extra_args = [
            [], # default is uncapped
            ["-datacarrier=0"], # no relay of datacarrier
            ["-datacarrier=1", f"-datacarriersize={CUSTOM_DATACARRIER_ARG}"],
            ["-datacarrier=1", "-datacarriersize=2"],
        ]

    def test_null_data_transaction(self, node: TestNode, data, success: bool, expected_error: str = "scriptpubkey") -> None:
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]
        data = [] if data is None else [data]
        tx.vout.append(CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN] + data)))
        tx.vout[0].nValue -= tx.get_vsize()  # simply pay 1sat/vbyte fee

        tx_hex = tx.serialize().hex()

        if success:
            self.wallet.sendrawtransaction(from_node=node, tx_hex=tx_hex)
            assert tx.txid_hex in node.getrawmempool(True), f'{tx_hex} not in mempool'
        else:
            assert_raises_rpc_error(-26, expected_error, self.wallet.sendrawtransaction, from_node=node, tx_hex=tx_hex)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Test that bare multisig is allowed by default. Do it here rather than create a new test for it.
        assert_equal(self.nodes[0].getmempoolinfo()["permitbaremultisig"], True)

        assert_equal(self.nodes[0].getmempoolinfo()["maxdatacarriersize"], MAX_OP_RETURN_RELAY)
        assert_equal(self.nodes[1].getmempoolinfo()["maxdatacarriersize"], 0)
        assert_equal(self.nodes[2].getmempoolinfo()["maxdatacarriersize"], CUSTOM_DATACARRIER_ARG)
        assert_equal(self.nodes[3].getmempoolinfo()["maxdatacarriersize"], 2)

        # OutputSizeLimit caps all OP_RETURN outputs to 83 bytes regardless of -datacarriersize.
        # 80 bytes data + 1 OP_RETURN + 2 pushdata opcodes = 83 bytes script (the maximum).
        custom_size_data = randbytes(CUSTOM_DATACARRIER_ARG - 3)  # 80 bytes data = 83 bytes script
        too_long_data = randbytes(CUSTOM_DATACARRIER_ARG - 2)  # 81 bytes data = 84 bytes script (exceeds limit)
        one_byte = randbytes(1)
        zero_bytes = randbytes(0)

        self.log.info("Testing a null data transaction at the maximum size (83 byte script).")
        self.test_null_data_transaction(node=self.nodes[0], data=custom_size_data, success=True)

        self.log.info("Testing a null data transaction exceeding OutputSizeLimit (84 byte script).")
        self.test_null_data_transaction(node=self.nodes[0], data=too_long_data, success=False, expected_error="bad-txns-vout-script-toolarge")

        self.log.info("Testing a null data transaction with -datacarrier=false.")
        self.test_null_data_transaction(node=self.nodes[1], data=custom_size_data, success=False, expected_error="datacarrier")

        self.log.info("Testing a null data transaction with a size larger than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], data=too_long_data, success=False, expected_error="datacarrier")

        self.log.info("Testing a null data transaction with a size equal to -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[2], data=custom_size_data, success=True)

        self.log.info("Testing a null data transaction with no data.")
        self.test_null_data_transaction(node=self.nodes[0], data=None, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=None, success=False, expected_error="datacarrier")
        self.test_null_data_transaction(node=self.nodes[2], data=None, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=None, success=True)

        self.log.info("Testing a null data transaction with zero bytes of data.")
        self.test_null_data_transaction(node=self.nodes[0], data=zero_bytes, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=zero_bytes, success=False, expected_error="datacarrier")
        self.test_null_data_transaction(node=self.nodes[2], data=zero_bytes, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=zero_bytes, success=True)

        self.log.info("Testing a null data transaction with one byte of data.")
        self.test_null_data_transaction(node=self.nodes[0], data=one_byte, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=one_byte, success=False, expected_error="datacarrier")
        self.test_null_data_transaction(node=self.nodes[2], data=one_byte, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=one_byte, success=False, expected_error="datacarrier")

if __name__ == '__main__':
    DataCarrierTest(__file__).main()
