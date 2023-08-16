#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# address.py
#
# This file encodes and decodes BASE58 P2PKH and P2SH addresses
#

from .script import hash256, hash160, CScript
from .util import hex_str_to_bytes

# Note unlike in bitcoin, this address isn't bech32 since we don't (at this time) support bech32.
ADDRESS_BCRT1_UNSPENDABLE = 'yVg3NBUHNEhgDceqwVUjsZHreC5PBHnUo9'
ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR = 'addr(yVg3NBUHNEhgDceqwVUjsZHreC5PBHnUo9)#e5kt0jtk'
ADDRESS_BCRT1_P2SH_OP_TRUE = '8zJctvfrzGZ5s1zQ3kagwyW1DsPYSQ4V2P'

chars = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'


def byte_to_base58(b, version):
    result = ''
    str = b.hex()
    str = chr(version).encode('latin-1').hex() + str
    checksum = hash256(hex_str_to_bytes(str)).hex()
    str += checksum[:8]
    value = int('0x'+str,0)
    while value > 0:
        result = chars[value % 58] + result
        value //= 58
    while (str[:2] == '00'):
        result = chars[0] + result
        str = str[2:]
    return result

# TODO: def base58_decode

def keyhash_to_p2pkh(hash, main = False):
    assert len(hash) == 20
    version = 76 if main else 140
    return byte_to_base58(hash, version)

def scripthash_to_p2sh(hash, main = False):
    assert len(hash) == 20
    version = 16 if main else 19
    return byte_to_base58(hash, version)

def key_to_p2pkh(key, main = False):
    key = check_key(key)
    return keyhash_to_p2pkh(hash160(key), main)

def script_to_p2sh(script, main = False):
    script = check_script(script)
    return scripthash_to_p2sh(hash160(script), main)

def check_key(key):
    if (type(key) is str):
        key = hex_str_to_bytes(key) # Assuming this is hex string
    if (type(key) is bytes and (len(key) == 33 or len(key) == 65)):
        return key
    assert False

def check_script(script):
    if (type(script) is str):
        script = hex_str_to_bytes(script) # Assuming this is hex string
    if (type(script) is bytes or type(script) is CScript):
        return script
    assert False
