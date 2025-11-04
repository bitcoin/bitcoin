#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Useful util functions for testing the wallet"""
from collections import namedtuple
import unittest

from test_framework.address import (
    byte_to_base58,
    key_to_p2pkh,
    key_to_p2sh_p2wpkh,
    key_to_p2wpkh,
)
from test_framework.key import ECKey
from test_framework.messages import (
    CTxIn,
    CTxInWitness,
    WITNESS_SCALE_FACTOR,
)
from test_framework.script_util import (
    key_to_p2pkh_script,
    key_to_p2wpkh_script,
    script_to_p2sh_script,
)

Key = namedtuple('Key', ['privkey',
                         'pubkey',
                         'p2pkh_script',
                         'p2pkh_addr',
                         'p2wpkh_script',
                         'p2wpkh_addr',
                         'p2sh_p2wpkh_script',
                         'p2sh_p2wpkh_redeem_script',
                         'p2sh_p2wpkh_addr'])


def get_generate_key():
    """Generate a fresh key

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    privkey, pubkey = generate_keypair(wif=True)
    return Key(privkey=privkey,
               pubkey=pubkey.hex(),
               p2pkh_script=key_to_p2pkh_script(pubkey).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey),
               p2wpkh_script=key_to_p2wpkh_script(pubkey).hex(),
               p2wpkh_addr=key_to_p2wpkh(pubkey),
               p2sh_p2wpkh_script=script_to_p2sh_script(key_to_p2wpkh_script(pubkey)).hex(),
               p2sh_p2wpkh_redeem_script=key_to_p2wpkh_script(pubkey).hex(),
               p2sh_p2wpkh_addr=key_to_p2sh_p2wpkh(pubkey))


def test_address(node, address, **kwargs):
    """Get address info for `address` and test whether the returned values are as expected."""
    addr_info = node.getaddressinfo(address)
    for key, value in kwargs.items():
        if value is None:
            if key in addr_info.keys():
                raise AssertionError("key {} unexpectedly returned in getaddressinfo.".format(key))
        elif addr_info[key] != value:
            raise AssertionError("key {} value {} did not match expected value {}".format(key, addr_info[key], value))

def bytes_to_wif(b, compressed=True):
    if compressed:
        b += b'\x01'
    return byte_to_base58(b, 239)

def generate_keypair(compressed=True, wif=False):
    """Generate a new random keypair and return the corresponding ECKey /
    bytes objects. The private key can also be provided as WIF (wallet
    import format) string instead, which is often useful for wallet RPC
    interaction."""
    privkey = ECKey()
    privkey.generate(compressed)
    pubkey = privkey.get_pubkey().get_bytes()
    if wif:
        privkey = bytes_to_wif(privkey.get_bytes(), compressed)
    return privkey, pubkey

def calculate_input_weight(scriptsig_hex, witness_stack_hex=None):
    """Given a scriptSig and a list of witness stack items for an input in hex format,
       calculate the total input weight. If the input has no witness data,
       `witness_stack_hex` can be set to None."""
    tx_in = CTxIn(scriptSig=bytes.fromhex(scriptsig_hex))
    witness_size = 0
    if witness_stack_hex is not None:
        tx_inwit = CTxInWitness()
        for witness_item_hex in witness_stack_hex:
            tx_inwit.scriptWitness.stack.append(bytes.fromhex(witness_item_hex))
        witness_size = len(tx_inwit.serialize())
    return len(tx_in.serialize()) * WITNESS_SCALE_FACTOR + witness_size

class WalletUnlock():
    """
    A context manager for unlocking a wallet with a passphrase and automatically locking it afterward.
    """

    MAXIMUM_TIMEOUT = 999000

    def __init__(self, wallet, passphrase, timeout=MAXIMUM_TIMEOUT):
        self.wallet = wallet
        self.passphrase = passphrase
        self.timeout = timeout

    def __enter__(self):
        self.wallet.walletpassphrase(self.passphrase, self.timeout)

    def __exit__(self, *args):
        _ = args
        self.wallet.walletlock()


class TestFrameworkWalletUtil(unittest.TestCase):
    def test_calculate_input_weight(self):
        SKELETON_BYTES = 32 + 4 + 4  # prevout-txid, prevout-index, sequence
        SMALL_LEN_BYTES = 1  # bytes needed for encoding scriptSig / witness item lengths < 253
        LARGE_LEN_BYTES = 3  # bytes needed for encoding scriptSig / witness item lengths >= 253

        # empty scriptSig, no witness
        self.assertEqual(calculate_input_weight(""),
                         (SKELETON_BYTES + SMALL_LEN_BYTES) * WITNESS_SCALE_FACTOR)
        self.assertEqual(calculate_input_weight("", None),
                         (SKELETON_BYTES + SMALL_LEN_BYTES) * WITNESS_SCALE_FACTOR)
        # small scriptSig, no witness
        scriptSig_small = "00"*252
        self.assertEqual(calculate_input_weight(scriptSig_small, None),
                         (SKELETON_BYTES + SMALL_LEN_BYTES + 252) * WITNESS_SCALE_FACTOR)
        # small scriptSig, empty witness stack
        self.assertEqual(calculate_input_weight(scriptSig_small, []),
                         (SKELETON_BYTES + SMALL_LEN_BYTES + 252) * WITNESS_SCALE_FACTOR + SMALL_LEN_BYTES)
        # large scriptSig, no witness
        scriptSig_large = "00"*253
        self.assertEqual(calculate_input_weight(scriptSig_large, None),
                         (SKELETON_BYTES + LARGE_LEN_BYTES + 253) * WITNESS_SCALE_FACTOR)
        # large scriptSig, empty witness stack
        self.assertEqual(calculate_input_weight(scriptSig_large, []),
                         (SKELETON_BYTES + LARGE_LEN_BYTES + 253) * WITNESS_SCALE_FACTOR + SMALL_LEN_BYTES)
        # empty scriptSig, 5 small witness stack items
        self.assertEqual(calculate_input_weight("", ["00", "11", "22", "33", "44"]),
                         ((SKELETON_BYTES + SMALL_LEN_BYTES) * WITNESS_SCALE_FACTOR) + SMALL_LEN_BYTES + 5 * SMALL_LEN_BYTES + 5)
        # empty scriptSig, 253 small witness stack items
        self.assertEqual(calculate_input_weight("", ["00"]*253),
                         ((SKELETON_BYTES + SMALL_LEN_BYTES) * WITNESS_SCALE_FACTOR) + LARGE_LEN_BYTES + 253 * SMALL_LEN_BYTES + 253)
        # small scriptSig, 3 large witness stack items
        self.assertEqual(calculate_input_weight(scriptSig_small, ["00"*253]*3),
                         ((SKELETON_BYTES + SMALL_LEN_BYTES + 252) * WITNESS_SCALE_FACTOR) + SMALL_LEN_BYTES + 3 * LARGE_LEN_BYTES + 3*253)
        # large scriptSig, 3 large witness stack items
        self.assertEqual(calculate_input_weight(scriptSig_large, ["00"*253]*3),
                         ((SKELETON_BYTES + LARGE_LEN_BYTES + 253) * WITNESS_SCALE_FACTOR) + SMALL_LEN_BYTES + 3 * LARGE_LEN_BYTES + 3*253)
