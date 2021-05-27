#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""A limited-functionality wallet, which may replace a real wallet in tests"""

from decimal import Decimal
from enum import Enum
from typing import Optional
from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.key import ECKey
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
    LegacySignatureHash,
    OP_CHECKSIG,
    OP_TRUE,
    OP_NOP,
    SIGHASH_ALL,
)
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes,
    satoshi_round,
)


class MiniWalletMode(Enum):
    """Determines the transaction type the MiniWallet is creating and spending.

    For most purposes, the default mode ADDRESS_OP_TRUE should be sufficient;
    it simply uses a fixed bech32 P2WSH address whose coins are spent with a
    witness stack of OP_TRUE, i.e. following an anyone-can-spend policy.
    However, if the transactions need to be modified by the user (e.g. prepending
    scriptSig for testing opcodes that are activated by a soft-fork), or the txs
    should contain an actual signature, the raw modes RAW_OP_TRUE and RAW_P2PK
    can be useful. Summary of modes:

                    |      output       |           |  tx is   | can modify |  needs
         mode       |    description    |  address  | standard | scriptSig  | signing
    ----------------+-------------------+-----------+----------+------------+----------
    ADDRESS_OP_TRUE | anyone-can-spend  |  bech32   |   yes    |    no      |   no
    RAW_OP_TRUE     | anyone-can-spend  |  - (raw)  |   no     |    yes     |   no
    RAW_P2PK        | pay-to-public-key |  - (raw)  |   yes    |    yes     |   yes
    """
    ADDRESS_OP_TRUE = 1
    RAW_OP_TRUE = 2
    RAW_P2PK = 3


class MiniWallet:
    def __init__(self, test_node, *, mode=MiniWalletMode.ADDRESS_OP_TRUE):
        self._test_node = test_node
        self._utxos = []
        self._priv_key = None
        self._address = None

        assert isinstance(mode, MiniWalletMode)
        if mode == MiniWalletMode.RAW_OP_TRUE:
            self._scriptPubKey = bytes(CScript([OP_TRUE]))
        elif mode == MiniWalletMode.RAW_P2PK:
            # use simple deterministic private key (k=1)
            self._priv_key = ECKey()
            self._priv_key.set((1).to_bytes(32, 'big'), True)
            pub_key = self._priv_key.get_pubkey()
            self._scriptPubKey = bytes(CScript([pub_key.get_bytes(), OP_CHECKSIG]))
        elif mode == MiniWalletMode.ADDRESS_OP_TRUE:
            self._address = ADDRESS_BCRT1_P2WSH_OP_TRUE
            self._scriptPubKey = hex_str_to_bytes(self._test_node.validateaddress(self._address)['scriptPubKey'])

    def scan_blocks(self, *, start=1, num):
        """Scan the blocks for self._address outputs and add them to self._utxos"""
        for i in range(start, start + num):
            block = self._test_node.getblock(blockhash=self._test_node.getblockhash(i), verbosity=2)
            for tx in block['tx']:
                self.scan_tx(tx)

    def scan_tx(self, tx):
        """Scan the tx for self._scriptPubKey outputs and add them to self._utxos"""
        for out in tx['vout']:
            if out['scriptPubKey']['hex'] == self._scriptPubKey.hex():
                self._utxos.append({'txid': tx['txid'], 'vout': out['n'], 'value': out['value']})

    def sign_tx(self, tx, fixed_length=True):
        """Sign tx that has been created by MiniWallet in P2PK mode"""
        assert self._priv_key is not None
        (sighash, err) = LegacySignatureHash(CScript(self._scriptPubKey), tx, 0, SIGHASH_ALL)
        assert err is None
        # for exact fee calculation, create only signatures with fixed size by default (>49.89% probability):
        # 65 bytes: high-R val (33 bytes) + low-S val (32 bytes)
        # with the DER header/skeleton data of 6 bytes added, this leads to a target size of 71 bytes
        der_sig = b''
        while not len(der_sig) == 71:
            der_sig = self._priv_key.sign_ecdsa(sighash)
            if not fixed_length:
                break
        tx.vin[0].scriptSig = CScript([der_sig + bytes(bytearray([SIGHASH_ALL]))])

    def generate(self, num_blocks):
        """Generate blocks with coinbase outputs to the internal address, and append the outputs to the internal list"""
        blocks = self._test_node.generatetodescriptor(num_blocks, f'raw({self._scriptPubKey.hex()})')
        for b in blocks:
            cb_tx = self._test_node.getblock(blockhash=b, verbosity=2)['tx'][0]
            self._utxos.append({'txid': cb_tx['txid'], 'vout': 0, 'value': cb_tx['vout'][0]['value']})
        return blocks

    def get_address(self):
        return self._address

    def get_utxo(self, *, txid: Optional[str]='', mark_as_spent=True):
        """
        Returns a utxo and marks it as spent (pops it from the internal list)

        Args:
        txid: get the first utxo we find from a specific transaction

        Note: Can be used to get the change output immediately after a send_self_transfer
        """
        index = -1  # by default the last utxo
        if txid:
            utxo = next(filter(lambda utxo: txid == utxo['txid'], self._utxos))
            index = self._utxos.index(utxo)
        if mark_as_spent:
            return self._utxos.pop(index)
        else:
            return self._utxos[index]

    def send_self_transfer(self, *, fee_rate=Decimal("0.003"), from_node, utxo_to_spend=None, locktime=0):
        """Create and send a tx with the specified fee_rate. Fee may be exact or at most one satoshi higher than needed."""
        tx = self.create_self_transfer(fee_rate=fee_rate, from_node=from_node, utxo_to_spend=utxo_to_spend)
        self.sendrawtransaction(from_node=from_node, tx_hex=tx['hex'])
        return tx

    def create_self_transfer(self, *, fee_rate=Decimal("0.003"), from_node, utxo_to_spend=None, mempool_valid=True, locktime=0):
        """Create and return a tx with the specified fee_rate. Fee may be exact or at most one satoshi higher than needed."""
        self._utxos = sorted(self._utxos, key=lambda k: k['value'])
        utxo_to_spend = utxo_to_spend or self._utxos.pop()  # Pick the largest utxo (if none provided) and hope it covers the fee
        vsize = Decimal(96)
        send_value = satoshi_round(utxo_to_spend['value'] - fee_rate * (vsize / 1000))
        fee = utxo_to_spend['value'] - send_value
        assert send_value > 0

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(utxo_to_spend['txid'], 16), utxo_to_spend['vout']))]
        tx.vout = [CTxOut(int(send_value * COIN), self._scriptPubKey)]
        tx.nLockTime = locktime
        if not self._address:
            # raw script
            if self._priv_key is not None:
                # P2PK, need to sign
                self.sign_tx(tx)
            else:
                # anyone-can-spend
                tx.vin[0].scriptSig = CScript([OP_NOP] * 35)  # pad to identical size
        else:
            tx.wit.vtxinwit = [CTxInWitness()]
            tx.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx_hex = tx.serialize().hex()

        tx_info = from_node.testmempoolaccept([tx_hex])[0]
        assert_equal(mempool_valid, tx_info['allowed'])
        if mempool_valid:
            # TODO: for P2PK, vsize is not constant due to varying scriptSig length,
            # so only check this for anyone-can-spend outputs right now
            if self._priv_key is None:
                assert_equal(tx_info['vsize'], vsize)
            assert_equal(tx_info['fees']['base'], fee)
        return {'txid': tx_info['txid'], 'wtxid': tx_info['wtxid'], 'hex': tx_hex, 'tx': tx}

    def sendrawtransaction(self, *, from_node, tx_hex):
        from_node.sendrawtransaction(tx_hex)
        self.scan_tx(from_node.decoderawtransaction(tx_hex))
