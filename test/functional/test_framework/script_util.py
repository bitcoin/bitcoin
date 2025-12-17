#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Useful Script constants and utils."""
import unittest

from copy import deepcopy

from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    ser_compact_size,
    sha256,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_1,
    OP_15,
    OP_16,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_ELSE,
    OP_ENDIF,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_HASH160,
    OP_IF,
    OP_RETURN,
    OP_TRUE,
    hash160,
)

from test_framework.util import assert_equal

# Maximum number of potentially executed legacy signature operations in validating a transaction.
MAX_STD_LEGACY_SIGOPS = 2_500

# Maximum number of sigops per standard P2SH redeemScript.
MAX_STD_P2SH_SIGOPS = 15

# To prevent a "tx-size-small" policy rule error, a transaction has to have a
# non-witness size of at least 65 bytes (MIN_STANDARD_TX_NONWITNESS_SIZE in
# src/policy/policy.h). Considering a Tx with the smallest possible single
# input (blank, empty scriptSig), and with an output omitting the scriptPubKey,
# we get to a minimum size of 60 bytes:
#
# Tx Skeleton: 4 [Version] + 1 [InCount] + 1 [OutCount] + 4 [LockTime] = 10 bytes
# Blank Input: 32 [PrevTxHash] + 4 [Index] + 1 [scriptSigLen] + 4 [SeqNo] = 41 bytes
# Output:      8 [Amount] + 1 [scriptPubKeyLen] = 9 bytes
#
# Hence, the scriptPubKey of the single output has to have a size of at
# least 5 bytes.
MIN_STANDARD_TX_NONWITNESS_SIZE = 65
MIN_PADDING = MIN_STANDARD_TX_NONWITNESS_SIZE - 10 - 41 - 9
assert MIN_PADDING == 5

# This script cannot be spent, allowing dust output values under
# standardness checks
DUMMY_MIN_OP_RETURN_SCRIPT = CScript([OP_RETURN] + ([OP_0] * (MIN_PADDING - 1)))
assert len(DUMMY_MIN_OP_RETURN_SCRIPT) == MIN_PADDING

PAY_TO_ANCHOR = CScript([OP_1, bytes.fromhex("4e73")])
ANCHOR_ADDRESS = "bcrt1pfeesnyr2tx"

def key_to_p2pk_script(key):
    key = check_key(key)
    return CScript([key, OP_CHECKSIG])


def keys_to_multisig_script(keys, *, k=None):
    n = len(keys)
    if k is None:  # n-of-n multisig by default
        k = n
    assert k <= n
    checked_keys = [check_key(key) for key in keys]
    return CScript([k] + checked_keys + [n, OP_CHECKMULTISIG])


def keyhash_to_p2pkh_script(hash):
    assert len(hash) == 20
    return CScript([OP_DUP, OP_HASH160, hash, OP_EQUALVERIFY, OP_CHECKSIG])


def scripthash_to_p2sh_script(hash):
    assert len(hash) == 20
    return CScript([OP_HASH160, hash, OP_EQUAL])


def key_to_p2pkh_script(key):
    key = check_key(key)
    return keyhash_to_p2pkh_script(hash160(key))


def script_to_p2sh_script(script):
    script = check_script(script)
    return scripthash_to_p2sh_script(hash160(script))


def key_to_p2sh_p2wpkh_script(key):
    key = check_key(key)
    p2shscript = CScript([OP_0, hash160(key)])
    return script_to_p2sh_script(p2shscript)


def program_to_witness_script(version, program):
    if isinstance(program, str):
        program = bytes.fromhex(program)
    assert 0 <= version <= 16
    assert 2 <= len(program) <= 40
    assert version > 0 or len(program) in [20, 32]
    return CScript([version, program])


def script_to_p2wsh_script(script):
    script = check_script(script)
    return program_to_witness_script(0, sha256(script))


def key_to_p2wpkh_script(key):
    key = check_key(key)
    return program_to_witness_script(0, hash160(key))


def script_to_p2sh_p2wsh_script(script):
    script = check_script(script)
    p2shscript = CScript([OP_0, sha256(script)])
    return script_to_p2sh_script(p2shscript)

def bulk_vout(tx, target_vsize):
    if target_vsize < tx.get_vsize():
        raise RuntimeError(f"target_vsize {target_vsize} is less than transaction virtual size {tx.get_vsize()}")
    # determine number of needed padding bytes
    dummy_vbytes = target_vsize - tx.get_vsize()
    # compensate for the increase of the compact-size encoded script length
    # (note that the length encoding of the unpadded output script needs one byte)
    dummy_vbytes -= len(ser_compact_size(dummy_vbytes)) - 1
    tx.vout[-1].scriptPubKey = CScript([OP_RETURN] + [OP_1] * dummy_vbytes)
    assert_equal(tx.get_vsize(), target_vsize)

def output_key_to_p2tr_script(key):
    assert len(key) == 32
    return program_to_witness_script(1, key)


def check_key(key):
    if isinstance(key, str):
        key = bytes.fromhex(key)  # Assuming this is hex string
    if isinstance(key, bytes) and (len(key) == 33 or len(key) == 65):
        return key
    assert False


def check_script(script):
    if isinstance(script, str):
        script = bytes.fromhex(script)  # Assuming this is hex string
    if isinstance(script, bytes) or isinstance(script, CScript):
        return script
    assert False


class ValidWitnessMalleatedTx:
    """
    Creates a valid witness malleation transaction test case:
    - Parent transaction with a script supporting 2 branches
    - 2 child transactions with the same txid but different wtxids
    """
    def __init__(self):
        hashlock = hash160(b'Preimage')
        self.witness_script = CScript([OP_IF, OP_HASH160, hashlock, OP_EQUAL, OP_ELSE, OP_TRUE, OP_ENDIF])

    def build_parent_tx(self, funding_txid, amount):
        # Create an unsigned parent transaction paying to the witness script.
        witness_program = sha256(self.witness_script)
        script_pubkey = CScript([OP_0, witness_program])

        parent = CTransaction()
        parent.vin.append(CTxIn(COutPoint(int(funding_txid, 16), 0), b""))
        parent.vout.append(CTxOut(int(amount), script_pubkey))
        return parent

    def build_malleated_children(self, signed_parent_txid, amount):
        # Create 2 valid children that differ only in witness data.
        # 1. Create a new transaction with witness solving first branch
        child_witness_script = CScript([OP_TRUE])
        child_witness_program = sha256(child_witness_script)
        child_script_pubkey = CScript([OP_0, child_witness_program])

        child_one = CTransaction()
        child_one.vin.append(CTxIn(COutPoint(int(signed_parent_txid, 16), 0), b""))
        child_one.vout.append(CTxOut(int(amount), child_script_pubkey))
        child_one.wit.vtxinwit.append(CTxInWitness())
        child_one.wit.vtxinwit[0].scriptWitness.stack = [b'Preimage', b'\x01', self.witness_script]

        # 2. Create another identical transaction with witness solving second branch
        child_two = deepcopy(child_one)
        child_two.wit.vtxinwit[0].scriptWitness.stack = [b'', self.witness_script]
        return child_one, child_two


class TestFrameworkScriptUtil(unittest.TestCase):
    def test_multisig(self):
        fake_pubkey = bytes([0]*33)
        # check correct encoding of P2MS script with n,k <= 16
        normal_ms_script = keys_to_multisig_script([fake_pubkey]*16, k=15)
        self.assertEqual(len(normal_ms_script), 1 + 16*34 + 1 + 1)
        self.assertTrue(normal_ms_script.startswith(bytes([OP_15])))
        self.assertTrue(normal_ms_script.endswith(bytes([OP_16, OP_CHECKMULTISIG])))

        # check correct encoding of P2MS script with n,k > 16
        max_ms_script = keys_to_multisig_script([fake_pubkey]*20, k=19)
        self.assertEqual(len(max_ms_script), 2 + 20*34 + 2 + 1)
        self.assertTrue(max_ms_script.startswith(bytes([1, 19])))  # using OP_PUSH1
        self.assertTrue(max_ms_script.endswith(bytes([1, 20, OP_CHECKMULTISIG])))
