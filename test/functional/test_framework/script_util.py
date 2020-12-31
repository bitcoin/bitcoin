#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Useful Script constants and utils."""
from test_framework.script import CScript, hash160, OP_DUP, OP_HASH160, OP_CHECKSIG, OP_EQUAL, OP_EQUALVERIFY
from test_framework.util import hex_str_to_bytes

# To prevent a "tx-size-small" policy rule error, a transaction has to have a
# size of at least 83 bytes (MIN_STANDARD_TX_SIZE in
# src/policy/policy.h). Considering a Tx with the smallest possible single
# input (blank, empty scriptSig), and with an output omitting the scriptPubKey,
# we get to a minimum size of 60 bytes:
#
# Tx Skeleton: 4 [Version] + 1 [InCount] + 1 [OutCount] + 4 [LockTime] = 10 bytes
# Blank Input: 32 [PrevTxHash] + 4 [Index] + 1 [scriptSigLen] + 4 [SeqNo] = 41 bytes
# Output:      8 [Amount] + 1 [scriptPubKeyLen] = 9 bytes
#
# Hence, the scriptPubKey of the single output has to have a size of at
# least 23 bytes, which corresponds to the size of a P2SH scriptPubKey.
# The following script constant consists of a single push of 22 bytes of 'a':
#   <PUSH_22> <22-bytes of 'a'>
# resulting in a 23-byte size. It should be used whenever (small) fake
# scriptPubKeys are needed, to guarantee that the minimum transaction size is
# met.
DUMMY_P2SH_SCRIPT = CScript([b'a' * 22])
DUMMY_2_P2SH_SCRIPT = CScript([b'b' * 22])

def keyhash_to_p2pkh_script(hash, main = False):
    assert len(hash) == 20
    return CScript([OP_DUP, OP_HASH160, hash, OP_EQUALVERIFY, OP_CHECKSIG])

def scripthash_to_p2sh_script(hash, main = False):
    assert len(hash) == 20
    return CScript([OP_HASH160, hash, OP_EQUAL])

def key_to_p2pkh_script(key, main = False):
    key = check_key(key)
    return keyhash_to_p2pkh_script(hash160(key), main)

def script_to_p2sh_script(script, main = False):
    script = check_script(script)
    return scripthash_to_p2sh_script(hash160(script), main)

def check_key(key):
    if isinstance(key, str):
        key = hex_str_to_bytes(key) # Assuming this is hex string
    if isinstance(key, bytes) and (len(key) == 33 or len(key) == 65):
        return key
    assert False

def check_script(script):
    if isinstance(script, str):
        script = hex_str_to_bytes(script) # Assuming this is hex string
    if isinstance(script, bytes) or isinstance(script, CScript):
        return script
    assert False
