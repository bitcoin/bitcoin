#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helper routines relevant for compact block filters (BIP158).
"""
from .crypto.siphash import siphash


def bip158_basic_element_hash(script_pub_key, N, block_hash):
    """ Calculates the ranged hash of a filter element as defined in BIP158:

    'The first step in the filter construction is hashing the variable-sized
    raw items in the set to the range [0, F), where F = N * M.'

    'The items are first passed through the pseudorandom function SipHash, which takes a
    128-bit key k and a variable-sized byte vector and produces a uniformly random 64-bit
    output. Implementations of this BIP MUST use the SipHash parameters c = 2 and d = 4.'

    'The parameter k MUST be set to the first 16 bytes of the hash (in standard
    little-endian representation) of the block for which the filter is constructed. This
    ensures the key is deterministic while still varying from block to block.'
    """
    M = 784931
    block_hash_bytes = bytes.fromhex(block_hash)[::-1]
    k0 = int.from_bytes(block_hash_bytes[0:8], 'little')
    k1 = int.from_bytes(block_hash_bytes[8:16], 'little')
    return (siphash(k0, k1, script_pub_key) * (N * M)) >> 64


def bip158_relevant_scriptpubkeys(node, block_hash):
    """ Determines the basic filter relevant scriptPubKeys as defined in BIP158:

    'A basic filter MUST contain exactly the following items for each transaction in a block:
       - The previous output script (the script being spent) for each input, except for
         the coinbase transaction.
       - The scriptPubKey of each output, aside from all OP_RETURN output scripts.'
    """
    spks = set()
    for tx in node.getblock(blockhash=block_hash, verbosity=3)['tx']:
        # gather prevout scripts
        for i in tx['vin']:
            if 'prevout' in i:
                spks.add(bytes.fromhex(i['prevout']['scriptPubKey']['hex']))
        # gather output scripts
        for o in tx['vout']:
            if o['scriptPubKey']['type'] != 'nulldata':
                spks.add(bytes.fromhex(o['scriptPubKey']['hex']))
    return spks
