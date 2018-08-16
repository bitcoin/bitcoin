#!/usr/bin/env python3
# Copyright (c) 2012-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Generate valid and invalid base58 address and private key test vectors.

Usage:
    gen_base58_test_vectors.py valid 50 > ../../src/test/data/base58_keys_valid.json
    gen_base58_test_vectors.py invalid 50 > ../../src/test/data/base58_keys_invalid.json
'''
# 2012 Wladimir J. van der Laan
# Released under MIT License
import os
from itertools import islice
from base58 import b58encode_chk, b58decode_chk, b58chars
import random
from binascii import b2a_hex

# key types
PUBKEY_ADDRESS = 0
SCRIPT_ADDRESS = 5
PUBKEY_ADDRESS_TEST = 111
SCRIPT_ADDRESS_TEST = 196
PRIVKEY = 128
PRIVKEY_TEST = 239

metadata_keys = ['isPrivkey', 'isTestnet', 'addrType', 'isCompressed']
# templates for valid sequences
templates = [
  # prefix, payload_size, suffix, metadata
  #                                  None = N/A
  ((PUBKEY_ADDRESS,),      20, (),   (False, False, 'pubkey', None)),
  ((SCRIPT_ADDRESS,),      20, (),   (False, False, 'script',  None)),
  ((PUBKEY_ADDRESS_TEST,), 20, (),   (False, True,  'pubkey', None)),
  ((SCRIPT_ADDRESS_TEST,), 20, (),   (False, True,  'script',  None)),
  ((PRIVKEY,),             32, (),   (True,  False, None,  False)),
  ((PRIVKEY,),             32, (1,), (True,  False, None,  True)),
  ((PRIVKEY_TEST,),        32, (),   (True,  True,  None,  False)),
  ((PRIVKEY_TEST,),        32, (1,), (True,  True,  None,  True))
]

def is_valid(v):
    '''Check vector v for validity'''
    result = b58decode_chk(v)
    if result is None:
        return False
    for template in templates:
        prefix = bytearray(template[0])
        suffix = bytearray(template[2])
        if result.startswith(prefix) and result.endswith(suffix):
            if (len(result) - len(prefix) - len(suffix)) == template[1]:
                return True
    return False

def gen_valid_vectors():
    '''Generate valid test vectors'''
    while True:
        for template in templates:
            prefix = bytearray(template[0])
            payload = bytearray(os.urandom(template[1]))
            suffix = bytearray(template[2])
            rv = b58encode_chk(prefix + payload + suffix)
            assert is_valid(rv)
            metadata = {x: y for x, y in zip(metadata_keys,template[3]) if y is not None}
            hexrepr = b2a_hex(payload)
            if isinstance(hexrepr, bytes):
                hexrepr = hexrepr.decode('utf8')
            yield (rv, hexrepr, metadata)

def gen_invalid_vector(template, corrupt_prefix, randomize_payload_size, corrupt_suffix):
    '''Generate possibly invalid vector'''
    if corrupt_prefix:
        prefix = os.urandom(1)
    else:
        prefix = bytearray(template[0])

    if randomize_payload_size:
        payload = os.urandom(max(int(random.expovariate(0.5)), 50))
    else:
        payload = os.urandom(template[1])

    if corrupt_suffix:
        suffix = os.urandom(len(template[2]))
    else:
        suffix = bytearray(template[2])

    return b58encode_chk(prefix + payload + suffix)

def randbool(p = 0.5):
    '''Return True with P(p)'''
    return random.random() < p

def gen_invalid_vectors():
    '''Generate invalid test vectors'''
    # start with some manual edge-cases
    yield "",
    yield "x",
    while True:
        # kinds of invalid vectors:
        #   invalid prefix
        #   invalid payload length
        #   invalid (randomized) suffix (add random data)
        #   corrupt checksum
        for template in templates:
            val = gen_invalid_vector(template, randbool(0.2), randbool(0.2), randbool(0.2))
            if random.randint(0,10)<1: # line corruption
                if randbool(): # add random character to end
                    val += random.choice(b58chars)
                else: # replace random character in the middle
                    n = random.randint(0, len(val))
                    val = val[0:n] + random.choice(b58chars) + val[n+1:]
            if not is_valid(val):
                yield val,

if __name__ == '__main__':
    import sys
    import json
    iters = {'valid':gen_valid_vectors, 'invalid':gen_invalid_vectors}
    try:
        uiter = iters[sys.argv[1]]
    except IndexError:
        uiter = gen_valid_vectors
    try:
        count = int(sys.argv[2])
    except IndexError:
        count = 0

    data = list(islice(uiter(), count))
    json.dump(data, sys.stdout, sort_keys=True, indent=4)
    sys.stdout.write('\n')

