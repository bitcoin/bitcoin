#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of passing `totalFee` to the bumpfee RPC."""
from decimal import Decimal

from test_framework.messages import BIP125_SEQUENCE_NUMBER
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class BumpFeeWithTotalFeeArgumentDeprecationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[
            "-walletrbf={}".format(i),
            "-mintxfee=0.00002",
        ] for i in range(self.num_nodes)]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        peer_node, rbf_node = self.nodes
        peer_node.generate(110)
        self.sync_all()
        peer_node.sendtoaddress(rbf_node.getnewaddress(), 0.001)
        self.sync_all()
        peer_node.generate(1)
        self.sync_all()
        rbfid = spend_one_input(rbf_node, peer_node.getnewaddress())

        self.log.info("Testing bumpfee with totalFee argument raises RPC error with deprecation message")
        assert_raises_rpc_error(
            -8,
            "totalFee argument has been deprecated and will be removed in 0.20. " +
            "Please use -deprecatedrpc=totalFee to continue using this argument until removal.",
            rbf_node.bumpfee, rbfid, {"totalFee": 2000})

        self.log.info("Testing bumpfee without totalFee argument does not raise")
        rbf_node.bumpfee(rbfid)

def spend_one_input(node, dest_address, change_size=Decimal("0.00049000")):
    tx_input = dict(sequence=BIP125_SEQUENCE_NUMBER,
                    **next(u for u in node.listunspent() if u["amount"] == Decimal("0.00100000")))
    destinations = {dest_address: Decimal("0.00050000")}
    destinations[node.getrawchangeaddress()] = change_size
    rawtx = node.createrawtransaction([tx_input], destinations)
    signedtx = node.signrawtransactionwithwallet(rawtx)
    txid = node.sendrawtransaction(signedtx["hex"])
    return txid

if __name__ == "__main__":
    BumpFeeWithTotalFeeArgumentDeprecationTest().main()
