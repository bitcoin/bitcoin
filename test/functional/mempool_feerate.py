#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from decimal import Decimal, getcontext

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    connect_nodes,
    create_confirmed_utxos
)


def get_signed_tx(node, inputs, feerate, value_out, type):
    """
    Generates a transaction to a {node}'s address, using speicified
    {inputs}, {feerate}, {value_out} and {type} of address.
    Returns the signed raw transaction.
    """
    address = node.getnewaddress(address_type=type)
    output = {address: Decimal(value_out)}
    tx = node.createrawtransaction(inputs, output)
    tx = node.signrawtransactionwithwallet(tx)
    tx_size = node.decoderawtransaction(tx["hex"])["weight"]
    output[address] -= Decimal(tx_size) * feerate / Decimal(1000)
    tx = node.createrawtransaction(inputs, output)
    return node.signrawtransactionwithwallet(tx)["hex"]


class FeerateTest(BitcoinTestFramework):
    """
    Sanity checks for the minimum relay feerate in weight units.
    """
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        getcontext().prec = 10
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)

        self.log.info("Generate some fresh utxos..")
        min_feerate = Decimal(250) * Decimal(10**-8)
        utxos = create_confirmed_utxos(min_feerate,
                                       self.nodes[0], 4)

        self.log.info("A transaction which pays exactly {} btc/kWU"
                      " should propagate".format(min_feerate))
        utxo = utxos.pop()
        inputs = [{"txid": utxo["txid"], "vout": utxo["vout"]}]
        amount = utxo["amount"]
        # Test all address types. `bech32` would be relayed on old versions,
        # but not `legacy` nor `p2sh-segwit`.
        for addr_type in ["legacy", "p2sh-segwit", "bech32"]:
            tx = get_signed_tx(self.nodes[0], inputs, min_feerate,
                               amount, addr_type)
            txid = self.nodes[0].sendrawtransaction(tx)
            dec = self.nodes[0].decoderawtransaction(tx)
            self.sync_mempools(wait=.2)
            assert txid in self.nodes[1].getrawmempool()
            inputs[0] = {"txid": txid, "vout": 0}
            amount -= dec["weight"] * min_feerate / Decimal(1000)

        self.log.info("A transaction below {} btc/kWU should not"
                      " propagate".format(min_feerate))
        for addr_type in ["legacy", "p2sh-segwit", "bech32"]:
            utxo = utxos.pop()
            inputs = [{"txid": utxo["txid"], "vout": utxo["vout"]}]
            tx = get_signed_tx(self.nodes[0], inputs,
                               min_feerate - Decimal(2 * 10**-8),
                               utxo["amount"], addr_type)
            # We should not be able to even send the transaction..
            assert_raises_rpc_error(-26, "min relay fee not met",
                                    self.nodes[0].sendrawtransaction, tx)
            # .. But if we try anyway ..
            self.stop_node(0)
            self.start_node(0, ["-minrelaytxfee=0.00000001"])
            txid = self.nodes[0].sendrawtransaction(tx)
            dec = self.nodes[0].decoderawtransaction(tx)
            # .. Our peer should not accept it.
            try:
                self.sync_mempools(wait=.2, timeout=2)
            except AssertionError:
                # The node #1 did not accept it (as expected)
                pass
            else:
                raise AssertionError("Transaction below minimum relay fee "
                                     "accepted by remote peer.")
            assert txid not in self.nodes[1].getrawmempool()
            self.stop_node(0)
            self.start_node(0)


if __name__ == '__main__':
    FeerateTest().main()
