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
            self._utxos.append({'txid': cb_tx['txid'], 'vout': 0, 'value': cb_tx['vout'][0]['value'], 'blockhash':b})
        return blocks

    def generate_random_outputs(self, utxo, num_outputs, confirmed=True):
        """Split up a utxo to create multiple outputs"""
        import random
        total_value = utxo['value']
        send_values = []
        for i in range(num_outputs):
            if i == num_outputs - 1:
                val = total_value
            else:
                val = satoshi_round(total_value * Decimal((1 / (num_outputs - i)) * random.uniform(0.5, 1.5)))
            send_values.append(val)
            total_value -= val
        self.send_self_transfer(from_node=self._test_node, utxo_to_spend=utxo, send_values=send_values)
        if confirmed:
            blockhash = self._test_node.generatetoaddress(1, self._address)[0]
            # Ignore the coinbase output since we only want confirmed outputs
            for i in range(num_outputs):
                self._utxos[-(i+1)]['blockhash'] = blockhash

    def get_confirmations(self, utxo):
        return self._test_node.getblock(blockhash=utxo['blockhash'], verbosity=1)['confirmations']

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

    def get_utxos(self, amount):
        """Removes and returns the last {amount} utxos from the wallet"""
        assert amount <= len(self._utxos)
        ret = self._utxos[-amount:]
        self._utxos = self._utxos[:-amount]
        return ret

    def send_self_transfer(self, *, fee_rate=Decimal("0.00003"), from_node, utxo_to_spend=None, send_values=None, nSequence=0, nVersion=1):
        """Create and send a tx with the specified fee_rate. Fee may be exact or at most one satoshi higher than needed."""
        self._utxos = sorted(self._utxos, key=lambda k: k['value'])
        utxo_to_spend = utxo_to_spend or self._utxos.pop()  # Pick the largest utxo (if none provided) and hope it covers the fee
        send_values = send_values or [utxo_to_spend['value']]
        assert sum(send_values) <= utxo_to_spend['value']
        vsize = Decimal(53 + 43 * len(send_values))
        tx = CTransaction()
        tx.nVersion = nVersion
        tx.vin = [CTxIn(COutPoint(int(utxo_to_spend['txid'], 16), utxo_to_spend['vout']), nSequence=nSequence)]

        # Distribute the fee evenly among all outputs
        fee_per_output = satoshi_round(fee_rate * vsize / 1000 / len(send_values))

        total_sent = 0
        for send_value in send_values:
            output_value = satoshi_round(send_value - fee_per_output)
            total_sent += output_value
            tx.vout.append(CTxOut(int(output_value * COIN), self._scriptPubKey))

        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx_hex = tx.serialize().hex()

        tx_info = from_node.testmempoolaccept([tx_hex])[0]
        for i, send_value in enumerate(send_values):
            self._utxos.append({'txid': tx_info['txid'], 'vout': i, 'value': satoshi_round(send_value - fee_per_output)})

        assert_equal(tx_info['txid'], from_node.sendrawtransaction(tx_hex))
        assert_equal(tx_info['vsize'], vsize)
        assert_equal(tx_info['fees']['base'], fee_per_output * len(send_values))
        return {'txid': tx_info['txid'], 'wtxid': tx_info['wtxid'], 'hex': tx_hex, 'value': sum(send_values)}
