#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test datacarrier functionality"""
from test_framework.messages import (
    CTxOut,
    MAX_DATACARRIER_RELAY,
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

# Historical defaults for test coverage
CUSTOM_OP_RETURN_SIZE = 83
CUSTOM_OP_RETURN_COUNT = 1

class DataCarrierTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 6
        self.extra_args = [
            [], # default
            ["-datacarrier=0"], # no relay of datacarrier
            ["-datacarrier=1", f"-datacarriersize={CUSTOM_OP_RETURN_SIZE - 1}"],
            ["-datacarrier=1", "-datacarriersize=2"],
            ["-datacarrier=1", "-datacarriersize=0"], # uncapped size
            ["-datacarrier=1", "-datacarriercount=0"], # uncapped count
        ]

    def test_null_data_transaction(self, node: TestNode, data, outputs: int, success: bool, expected_error = "datacarrier") -> None:
        tx = self.wallet.create_self_transfer(fee_rate=0)["tx"]
        data = [] if data is None else [data]
        for i in range(outputs): # repeated data in each output
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

        # By default, only 80 bytes are used for data (+1 for OP_RETURN, +2 for the pushdata opcodes).
        default_size_data = randbytes(CUSTOM_OP_RETURN_SIZE - 3)
        too_long_data = randbytes(CUSTOM_OP_RETURN_SIZE - 2)
        small_data = randbytes(CUSTOM_OP_RETURN_SIZE - 4)

        extremely_long_data = randbytes(MAX_DATACARRIER_RELAY - 200)
        over_maximum_data = randbytes(MAX_DATACARRIER_RELAY)

        one_byte = randbytes(1)
        zero_bytes = randbytes(0)

        assert_equal(self.nodes[0].getmempoolinfo()["datacarrieraccept"], 1)
        assert_equal(self.nodes[0].getmempoolinfo()["datacarriersizelimit"], CUSTOM_OP_RETURN_SIZE)
        assert_equal(self.nodes[0].getmempoolinfo()["datacarriercountlimit"], CUSTOM_OP_RETURN_COUNT)

        assert_equal(self.nodes[1].getmempoolinfo()["datacarrieraccept"], 0)
        assert_equal(self.nodes[1].getmempoolinfo()["datacarriersizelimit"], 0)
        assert_equal(self.nodes[1].getmempoolinfo()["datacarriercountlimit"], 0)

        assert_equal(self.nodes[2].getmempoolinfo()["datacarrieraccept"], 1)
        assert_equal(self.nodes[2].getmempoolinfo()["datacarriersizelimit"], CUSTOM_OP_RETURN_SIZE - 1)
        assert_equal(self.nodes[2].getmempoolinfo()["datacarriercountlimit"], CUSTOM_OP_RETURN_COUNT)

        assert_equal(self.nodes[3].getmempoolinfo()["datacarrieraccept"], 1)
        assert_equal(self.nodes[3].getmempoolinfo()["datacarriersizelimit"], 2)
        assert_equal(self.nodes[3].getmempoolinfo()["datacarriercountlimit"], CUSTOM_OP_RETURN_COUNT)

        assert_equal(self.nodes[4].getmempoolinfo()["datacarrieraccept"], 1)
        assert_equal(self.nodes[4].getmempoolinfo()["datacarriersizelimit"], 0)
        assert_equal(self.nodes[4].getmempoolinfo()["datacarriercountlimit"], CUSTOM_OP_RETURN_COUNT)

        assert_equal(self.nodes[5].getmempoolinfo()["datacarrieraccept"], 1)
        assert_equal(self.nodes[5].getmempoolinfo()["datacarriersizelimit"], CUSTOM_OP_RETURN_SIZE)
        assert_equal(self.nodes[5].getmempoolinfo()["datacarriercountlimit"], 0)

        self.log.info("Testing a null data transaction with a size equal to -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[0], data=default_size_data, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=default_size_data, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=default_size_data, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[3], data=default_size_data, outputs=1, success=False)

        self.log.info("Testing a null data transaction with a size larger than accepted by -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[0], data=too_long_data, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[1], data=too_long_data, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=too_long_data, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[3], data=too_long_data, outputs=1, success=False)

        self.log.info("Testing a null data transaction with a smaller size than -datacarriersize.")
        self.test_null_data_transaction(node=self.nodes[0], data=small_data, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=small_data, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=small_data, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=small_data, outputs=1, success=False)

        self.log.info("Testing a null data transaction with no data.")
        self.test_null_data_transaction(node=self.nodes[0], data=None, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=None, outputs=1, success=False)
        # TODO
        # self.test_null_data_transaction(node=self.nodes[2], data=None, outputs=1, success=True)
        # self.test_null_data_transaction(node=self.nodes[3], data=None, outputs=1, success=True)

        self.log.info("Testing a null data transaction with zero bytes of data.")
        self.test_null_data_transaction(node=self.nodes[0], data=zero_bytes, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=zero_bytes, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=zero_bytes, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=zero_bytes, outputs=1, success=True)

        self.log.info("Testing a null data transaction with one byte of data.")
        self.test_null_data_transaction(node=self.nodes[0], data=one_byte, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[1], data=one_byte, outputs=1, success=False)
        self.test_null_data_transaction(node=self.nodes[2], data=one_byte, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[3], data=one_byte, outputs=1, success=False)

        self.log.info("Testing a null data transaction succeeds regardless of size.")
        self.test_null_data_transaction(node=self.nodes[4], data=extremely_long_data, outputs=1, success=True)
        self.test_null_data_transaction(node=self.nodes[4], data=over_maximum_data, outputs=1, success=False, expected_error="tx-size")

        # TODO
        self.log.info("Testing limits for null data outputs in a transaction.")
        self.test_null_data_transaction(node=self.nodes[5], data=None, outputs=int(MAX_DATACARRIER_RELAY/100), success=True)
        self.test_null_data_transaction(node=self.nodes[5], data=None, outputs=int(MAX_DATACARRIER_RELAY/10), success=False, expected_error="tx-size")

if __name__ == '__main__':
    DataCarrierTest(__file__).main()
