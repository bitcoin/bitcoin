#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""A limited-functionality wallet, which may replace a real wallet in tests"""

from copy import deepcopy
from decimal import Decimal
from enum import Enum
from random import choice
from typing import Optional
from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.descriptors import descsum_create
from test_framework.key import ECKey
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    tx_from_hex,
)
from test_framework.script import (
    CScript,
    LegacySignatureHash,
    OP_TRUE,
    OP_NOP,
    SIGHASH_ALL,
)
from test_framework.script_util import (
    key_to_p2pk_script,
    key_to_p2wpkh_script,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)

DEFAULT_FEE = Decimal("0.0001")

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
            self._scriptPubKey = key_to_p2pk_script(pub_key.get_bytes())
        elif mode == MiniWalletMode.ADDRESS_OP_TRUE:
            self._address = ADDRESS_BCRT1_P2WSH_OP_TRUE
            self._scriptPubKey = bytes.fromhex(self._test_node.validateaddress(self._address)['scriptPubKey'])

    def rescan_utxos(self):
        """Drop all utxos and rescan the utxo set"""
        self._utxos = []
        res = self._test_node.scantxoutset(action="start", scanobjects=[self.get_descriptor()])
        assert_equal(True, res['success'])
        for utxo in res['unspents']:
            self._utxos.append({'txid': utxo['txid'], 'vout': utxo['vout'], 'value': utxo['amount']})

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
        blocks = self._test_node.generatetodescriptor(num_blocks, self.get_descriptor())
        for b in blocks:
            cb_tx = self._test_node.getblock(blockhash=b, verbosity=2)['tx'][0]
            self._utxos.append({'txid': cb_tx['txid'], 'vout': 0, 'value': cb_tx['vout'][0]['value']})
        return blocks

    def get_descriptor(self):
        return descsum_create(f'raw({self._scriptPubKey.hex()})')

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

    def send_self_transfer(self, **kwargs):
        """Create and send a tx with the specified fee_rate. Fee may be exact or at most one satoshi higher than needed."""
        tx = self.create_self_transfer(**kwargs)
        self.sendrawtransaction(from_node=kwargs['from_node'], tx_hex=tx['hex'])
        return tx

    def send_to(self, *, from_node, scriptPubKey, amount, fee=1000):
        """
        Create and send a tx with an output to a given scriptPubKey/amount,
        plus a change output to our internal address. To keep things simple, a
        fixed fee given in Satoshi is used.

        Note that this method fails if there is no single internal utxo
        available that can cover the cost for the amount and the fixed fee
        (the utxo with the largest value is taken).

        Returns a tuple (txid, n) referring to the created external utxo outpoint.
        """
        tx = self.create_self_transfer(from_node=from_node, fee_rate=0, mempool_valid=False)['tx']
        assert_greater_than_or_equal(tx.vout[0].nValue, amount + fee)
        tx.vout[0].nValue -= (amount + fee)           # change output -> MiniWallet
        tx.vout.append(CTxOut(amount, scriptPubKey))  # arbitrary output -> to be returned
        txid = self.sendrawtransaction(from_node=from_node, tx_hex=tx.serialize().hex())
        return txid, 1

    def create_self_transfer(self, *, fee_rate=Decimal("0.003"), from_node, utxo_to_spend=None, mempool_valid=True, locktime=0, sequence=0):
        """Create and return a tx with the specified fee_rate. Fee may be exact or at most one satoshi higher than needed."""
        self._utxos = sorted(self._utxos, key=lambda k: k['value'])
        utxo_to_spend = utxo_to_spend or self._utxos.pop()  # Pick the largest utxo (if none provided) and hope it covers the fee
        if self._priv_key is None:
            vsize = Decimal(96)  # anyone-can-spend
        else:
            vsize = Decimal(168)  # P2PK (73 bytes scriptSig + 35 bytes scriptPubKey + 60 bytes other)
        send_value = int(COIN * (utxo_to_spend['value'] - fee_rate * (vsize / 1000)))
        assert send_value > 0

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(utxo_to_spend['txid'], 16), utxo_to_spend['vout']), nSequence=sequence)]
        tx.vout = [CTxOut(send_value, self._scriptPubKey)]
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
            assert_equal(tx_info['vsize'], vsize)
            assert_equal(tx_info['fees']['base'], utxo_to_spend['value'] - Decimal(send_value) / COIN)
        return {'txid': tx_info['txid'], 'wtxid': tx_info['wtxid'], 'hex': tx_hex, 'tx': tx}

    def sendrawtransaction(self, *, from_node, tx_hex):
        txid = from_node.sendrawtransaction(tx_hex)
        self.scan_tx(from_node.decoderawtransaction(tx_hex))
        return txid


def random_p2wpkh():
    """Generate a random P2WPKH scriptPubKey. Can be used when a random destination is needed,
    but no compiled wallet is available (e.g. as replacement to the getnewaddress RPC)."""
    key = ECKey()
    key.generate()
    return key_to_p2wpkh_script(key.get_pubkey().get_bytes())


def make_chain(node, address, privkeys, parent_txid, parent_value, n=0, parent_locking_script=None, fee=DEFAULT_FEE):
    """Build a transaction that spends parent_txid.vout[n] and produces one output with
    amount = parent_value with a fee deducted.
    Return tuple (CTransaction object, raw hex, nValue, scriptPubKey of the output created).
    """
    inputs = [{"txid": parent_txid, "vout": n}]
    my_value = parent_value - fee
    outputs = {address : my_value}
    rawtx = node.createrawtransaction(inputs, outputs)
    prevtxs = [{
        "txid": parent_txid,
        "vout": n,
        "scriptPubKey": parent_locking_script,
        "amount": parent_value,
    }] if parent_locking_script else None
    signedtx = node.signrawtransactionwithkey(hexstring=rawtx, privkeys=privkeys, prevtxs=prevtxs)
    assert signedtx["complete"]
    tx = tx_from_hex(signedtx["hex"])
    return (tx, signedtx["hex"], my_value, tx.vout[0].scriptPubKey.hex())

def create_child_with_parents(node, address, privkeys, parents_tx, values, locking_scripts, fee=DEFAULT_FEE):
    """Creates a transaction that spends the first output of each parent in parents_tx."""
    num_parents = len(parents_tx)
    total_value = sum(values)
    inputs = [{"txid": tx.rehash(), "vout": 0} for tx in parents_tx]
    outputs = {address : total_value - fee}
    rawtx_child = node.createrawtransaction(inputs, outputs)
    prevtxs = []
    for i in range(num_parents):
        prevtxs.append({"txid": parents_tx[i].rehash(), "vout": 0, "scriptPubKey": locking_scripts[i], "amount": values[i]})
    signedtx_child = node.signrawtransactionwithkey(hexstring=rawtx_child, privkeys=privkeys, prevtxs=prevtxs)
    assert signedtx_child["complete"]
    return signedtx_child["hex"]

def create_raw_chain(node, first_coin, address, privkeys, chain_length=25):
    """Helper function: create a "chain" of chain_length transactions. The nth transaction in the
    chain is a child of the n-1th transaction and parent of the n+1th transaction.
    """
    parent_locking_script = None
    txid = first_coin["txid"]
    chain_hex = []
    chain_txns = []
    value = first_coin["amount"]

    for _ in range(chain_length):
        (tx, txhex, value, parent_locking_script) = make_chain(node, address, privkeys, txid, value, 0, parent_locking_script)
        txid = tx.rehash()
        chain_hex.append(txhex)
        chain_txns.append(tx)

    return (chain_hex, chain_txns)

def bulk_transaction(tx, node, target_weight, privkeys, prevtxs=None):
    """Pad a transaction with extra outputs until it reaches a target weight (or higher).
    returns CTransaction object
    """
    tx_heavy = deepcopy(tx)
    assert_greater_than_or_equal(target_weight, tx_heavy.get_weight())
    while tx_heavy.get_weight() < target_weight:
        random_spk = "6a4d0200"  # OP_RETURN OP_PUSH2 512 bytes
        for _ in range(512*2):
            random_spk += choice("0123456789ABCDEF")
        tx_heavy.vout.append(CTxOut(0, bytes.fromhex(random_spk)))
    # Re-sign the transaction
    if privkeys:
        signed = node.signrawtransactionwithkey(tx_heavy.serialize().hex(), privkeys, prevtxs)
        return tx_from_hex(signed["hex"])
    # OP_TRUE
    tx_heavy.wit.vtxinwit = [CTxInWitness()]
    tx_heavy.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
    return tx_heavy
