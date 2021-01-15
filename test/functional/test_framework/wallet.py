#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""A limited-functionality wallet, which may replace a real wallet in tests"""

from decimal import Decimal
from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes,
    satoshi_round,
)


class MiniWallet:
    def __init__(self, test_node):
        self._test_node = test_node
        self._utxos = []
        self._address = ADDRESS_BCRT1_P2WSH_OP_TRUE
        self._scriptPubKey = hex_str_to_bytes(self._test_node.validateaddress(self._address)['scriptPubKey'])

    def generate(self, num_blocks):
        """Generate blocks with coinbase outputs to the internal address, and append the outputs to the internal list"""
        blocks = self._test_node.generatetoaddress(num_blocks, self._address)
        for b in blocks:
            cb_tx = self._test_node.getblock(blockhash=b, verbosity=2)['tx'][0]
            self._utxos.append({'txid': cb_tx['txid'], 'vout': 0, 'value': cb_tx['vout'][0]['value']})
        return blocks

    def get_utxo(self, *, txid=''):
        """
        Returns a utxo and marks it as spent (pops it from the internal list)

        Args:
        txid (string), optional: get the first utxo we find from a specific transaction

        Note: Can be used to get the change output immediately after a send_self_transfer
        """
        index = -1  # by default the last utxo
        if txid:
            utxo = next(filter(lambda utxo: txid == utxo['txid'], self._utxos))
            index = self._utxos.index(utxo)
        return self._utxos.pop(index)

    def send_self_transfer(self, *, fee_rate=Decimal("0.003"), from_node, utxo_to_spend=None):
        """Create and send a tx with the specified fee_rate. Fee may be exact or at most one satoshi higher than needed."""
        self._utxos = sorted(self._utxos, key=lambda k: k['value'])
        utxo_to_spend = utxo_to_spend or self._utxos.pop()  # Pick the largest utxo (if none provided) and hope it covers the fee
        vsize = Decimal(96)
        send_value = satoshi_round(utxo_to_spend['value'] - fee_rate * (vsize / 1000))
        fee = utxo_to_spend['value'] - send_value
        assert send_value > 0

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(utxo_to_spend['txid'], 16), utxo_to_spend['vout']))]
        tx.vout = [CTxOut(int(send_value * COIN), self._scriptPubKey)]
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx_hex = tx.serialize().hex()

        tx_info = from_node.testmempoolaccept([tx_hex])[0]
        self._utxos.append({'txid': tx_info['txid'], 'vout': 0, 'value': send_value})
        from_node.sendrawtransaction(tx_hex)
        assert_equal(tx_info['vsize'], vsize)
        assert_equal(tx_info['fees']['base'], fee)
        return {'txid': tx_info['txid'], 'wtxid': tx_info['wtxid'], 'hex': tx_hex}
