#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Encode and decode Bitcoin addresses.

- base58 P2PKH and P2SH addresses.
- bech32 segwit v0 P2WPKH and P2WSH addresses.
- bech32m segwit v1 P2TR addresses."""

import enum
import unittest

from .script import (
    CScript,
    OP_0,
    OP_TRUE,
    hash160,
    hash256,
    sha256,
    taproot_construct,
)
from .util import assert_equal
from test_framework.script_util import (
    keyhash_to_p2pkh_script,
    program_to_witness_script,
    scripthash_to_p2sh_script,
)
from test_framework.segwit_addr import (
    decode_segwit_address,
    encode_segwit_address,
)


ADDRESS_BCRT1_UNSPENDABLE = 'bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj'
ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR = 'addr(bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj)#juyq9d97'
# Coins sent to this address can be spent with a witness stack of just OP_TRUE
ADDRESS_BCRT1_P2WSH_OP_TRUE = 'bcrt1qft5p2uhsdcdc3l2ua4ap5qqfg4pjaqlp250x7us7a8qqhrxrxfsqseac85'


class AddressType(enum.Enum):
    bech32 = 'bech32'
    p2sh_segwit = 'p2sh-segwit'
    legacy = 'legacy'  # P2PKH


b58chars = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'


def create_deterministic_address_bcrt1_p2tr_op_true():
    """
    Generates a deterministic bech32m address (segwit v1 output) that
    can be spent with a witness stack of OP_TRUE and the control block
    with internal public key (script-path spending).

    Returns a tuple with the generated address and the internal key.
    """
    internal_key = (1).to_bytes(32, 'big')
    address = output_key_to_p2tr(taproot_construct(internal_key, [(None, CScript([OP_TRUE]))]).output_pubkey)
    assert_equal(address, 'bcrt1p9yfmy5h72durp7zrhlw9lf7jpwjgvwdg0jr0lqmmjtgg83266lqsekaqka')
    return (address, internal_key)


def byte_to_base58(b, version):
    result = ''
    b = bytes([version]) + b  # prepend version
    b += hash256(b)[:4]       # append checksum
    value = int.from_bytes(b, 'big')
    while value > 0:
        result = b58chars[value % 58] + result
        value //= 58
    while b[0] == 0:
        result = b58chars[0] + result
        b = b[1:]
    return result


def base58_to_byte(s):
    """Converts a base58-encoded string to its data and version.

    Throws if the base58 checksum is invalid."""
    if not s:
        return b''
    n = 0
    for c in s:
        n *= 58
        assert c in b58chars
        digit = b58chars.index(c)
        n += digit
    h = '%x' % n
    if len(h) % 2:
        h = '0' + h
    res = n.to_bytes((n.bit_length() + 7) // 8, 'big')
    pad = 0
    for c in s:
        if c == b58chars[0]:
            pad += 1
        else:
            break
    res = b'\x00' * pad + res

    if hash256(res[:-4])[:4] != res[-4:]:
        raise ValueError('Invalid Base58Check checksum')

    return res[1:-4], int(res[0])


def keyhash_to_p2pkh(hash, main=False):
    assert len(hash) == 20
    version = 0 if main else 111
    return byte_to_base58(hash, version)

def scripthash_to_p2sh(hash, main=False):
    assert len(hash) == 20
    version = 5 if main else 196
    return byte_to_base58(hash, version)

def key_to_p2pkh(key, main=False):
    key = check_key(key)
    return keyhash_to_p2pkh(hash160(key), main)

def script_to_p2sh(script, main=False):
    script = check_script(script)
    return scripthash_to_p2sh(hash160(script), main)

def key_to_p2sh_p2wpkh(key, main=False):
    key = check_key(key)
    p2shscript = CScript([OP_0, hash160(key)])
    return script_to_p2sh(p2shscript, main)

def program_to_witness(version, program, main=False):
    if (type(program) is str):
        program = bytes.fromhex(program)
    assert 0 <= version <= 16
    assert 2 <= len(program) <= 40
    assert version > 0 or len(program) in [20, 32]
    return encode_segwit_address("bc" if main else "bcrt", version, program)

def script_to_p2wsh(script, main=False):
    script = check_script(script)
    return program_to_witness(0, sha256(script), main)

def key_to_p2wpkh(key, main=False):
    key = check_key(key)
    return program_to_witness(0, hash160(key), main)

def script_to_p2sh_p2wsh(script, main=False):
    script = check_script(script)
    p2shscript = CScript([OP_0, sha256(script)])
    return script_to_p2sh(p2shscript, main)

def output_key_to_p2tr(key, main=False):
    assert len(key) == 32
    return program_to_witness(1, key, main)

def check_key(key):
    if (type(key) is str):
        key = bytes.fromhex(key)  # Assuming this is hex string
    if (type(key) is bytes and (len(key) == 33 or len(key) == 65)):
        return key
    assert False

def check_script(script):
    if (type(script) is str):
        script = bytes.fromhex(script)  # Assuming this is hex string
    if (type(script) is bytes or type(script) is CScript):
        return script
    assert False


def bech32_to_bytes(address):
    hrp = address.split('1')[0]
    if hrp not in ['bc', 'tb', 'bcrt']:
        return (None, None)
    version, payload = decode_segwit_address(hrp, address)
    if version is None:
        return (None, None)
    return version, bytearray(payload)


def address_to_scriptpubkey(address):
    """Converts a given address to the corresponding output script (scriptPubKey)."""
    version, payload = bech32_to_bytes(address)
    if version is not None:
        return program_to_witness_script(version, payload) # testnet segwit scriptpubkey
    payload, version = base58_to_byte(address)
    if version == 111:  # testnet pubkey hash
        return keyhash_to_p2pkh_script(payload)
    elif version == 196:  # testnet script hash
        return scripthash_to_p2sh_script(payload)
    # TODO: also support other address formats
    else:
        assert False


class TestFrameworkScript(unittest.TestCase):
    def test_base58encodedecode(self):
        def check_base58(data, version):
            self.assertEqual(base58_to_byte(byte_to_base58(data, version)), (data, version))

        check_base58(bytes.fromhex('1f8ea1702a7bd4941bca0941b852c4bbfedb2e05'), 111)
        check_base58(bytes.fromhex('3a0b05f4d7f66c3ba7009f453530296c845cc9cf'), 111)
        check_base58(bytes.fromhex('41c1eaf111802559bad61b60d62b1f897c63928a'), 111)
        check_base58(bytes.fromhex('0041c1eaf111802559bad61b60d62b1f897c63928a'), 111)
        check_base58(bytes.fromhex('000041c1eaf111802559bad61b60d62b1f897c63928a'), 111)
        check_base58(bytes.fromhex('00000041c1eaf111802559bad61b60d62b1f897c63928a'), 111)
        check_base58(bytes.fromhex('1f8ea1702a7bd4941bca0941b852c4bbfedb2e05'), 0)
        check_base58(bytes.fromhex('3a0b05f4d7f66c3ba7009f453530296c845cc9cf'), 0)
        check_base58(bytes.fromhex('41c1eaf111802559bad61b60d62b1f897c63928a'), 0)
        check_base58(bytes.fromhex('0041c1eaf111802559bad61b60d62b1f897c63928a'), 0)
        check_base58(bytes.fromhex('000041c1eaf111802559bad61b60d62b1f897c63928a'), 0)
        check_base58(bytes.fromhex('00000041c1eaf111802559bad61b60d62b1f897c63928a'), 0)


    def test_bech32_decode(self):
        def check_bech32_decode(payload, version):
            hrp = "tb"
            self.assertEqual(bech32_to_bytes(encode_segwit_address(hrp, version, payload)), (version, payload))

        check_bech32_decode(bytes.fromhex('36e3e2a33f328de12e4b43c515a75fba2632ecc3'), 0)
        check_bech32_decode(bytes.fromhex('823e9790fc1d1782321140d4f4aa61aabd5e045b'), 0)
        check_bech32_decode(bytes.fromhex('79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798'), 1)
        check_bech32_decode(bytes.fromhex('39cf8ebd95134f431c39db0220770bd127f5dd3cc103c988b7dcd577ae34e354'), 1)
        check_bech32_decode(bytes.fromhex('708244006d27c757f6f1fc6f853b6ec26268b727866f7ce632886e34eb5839a3'), 1)
        check_bech32_decode(bytes.fromhex('616211ab00dffe0adcb6ce258d6d3fd8cbd901e2'), 0)
        check_bech32_decode(bytes.fromhex('b6a7c98b482d7fb21c9fa8e65692a0890410ff22'), 0)
        check_bech32_decode(bytes.fromhex('f0c2109cb1008cfa7b5a09cc56f7267cd8e50929'), 0)
