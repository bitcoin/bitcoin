#!/usr/bin/env python3
# Copyright (c) 2012-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Generate valid and invalid base58 address and private key test vectors.

Usage:
    PYTHONPATH=../../test/functional/test_framework ./gen_key_io_test_vectors.py valid 50 > ../../src/test/data/key_io_valid.json
    PYTHONPATH=../../test/functional/test_framework ./gen_key_io_test_vectors.py invalid 50 > ../../src/test/data/key_io_invalid.json
'''
# 2012 Wladimir J. van der Laan
# Released under MIT License
import os
from itertools import islice
from base58 import b58encode_chk, b58decode_chk, b58chars
import random

# key types
PUBKEY_ADDRESS = 76
SCRIPT_ADDRESS = 16
PUBKEY_ADDRESS_TEST = 140
SCRIPT_ADDRESS_TEST = 19
PUBKEY_ADDRESS_REGTEST = 140
SCRIPT_ADDRESS_REGTEST = 19
PRIVKEY = 204
PRIVKEY_TEST = 239
PRIVKEY_REGTEST = 239

# script
OP_0 = 0x00
OP_1 = 0x51
OP_2 = 0x52
OP_16 = 0x60
OP_DUP = 0x76
OP_EQUAL = 0x87
OP_EQUALVERIFY = 0x88
OP_HASH160 = 0xa9
OP_CHECKSIG = 0xac
pubkey_prefix = (OP_DUP, OP_HASH160, 20)
pubkey_suffix = (OP_EQUALVERIFY, OP_CHECKSIG)
script_prefix = (OP_HASH160, 20)
script_suffix = (OP_EQUAL,)

metadata_keys = ['isPrivkey', 'chain', 'isCompressed', 'tryCaseFlip']
# templates for valid sequences
templates = [
  # prefix, payload_size, suffix, metadata, output_prefix, output_suffix
  #                                  None = N/A
  ((PUBKEY_ADDRESS,),         20, (),   (False, 'main',    None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS,),         20, (),   (False, 'main',    None,  None), script_prefix, script_suffix),
  ((PUBKEY_ADDRESS_TEST,),    20, (),   (False, 'test',    None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS_TEST,),    20, (),   (False, 'test',    None,  None), script_prefix, script_suffix),
  ((PUBKEY_ADDRESS_REGTEST,), 20, (),   (False, 'regtest', None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS_REGTEST,), 20, (),   (False, 'regtest', None,  None), script_prefix, script_suffix),
  ((PRIVKEY,),                32, (),   (True,  'main',    False, None), (),            ()),
  ((PRIVKEY,),                32, (1,), (True,  'main',    True,  None), (),            ()),
  ((PRIVKEY_TEST,),           32, (),   (True,  'test',    False, None), (),            ()),
  ((PRIVKEY_TEST,),           32, (1,), (True,  'test',    True,  None), (),            ()),
  ((PRIVKEY_REGTEST,),        32, (),   (True,  'regtest', False, None), (),            ()),
  ((PRIVKEY_REGTEST,),        32, (1,), (True,  'regtest', True,  None), (),            ())
]

def is_valid(v):
    '''Check vector v for validity'''
    if len(set(v) - set(b58chars)) > 0:
        return False
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

def gen_valid_base58_vector(template):
    '''Generate valid base58 vector'''
    prefix = bytearray(template[0])
    payload = bytearray(os.urandom(template[1]))
    suffix = bytearray(template[2])
    dst_prefix = bytearray(template[4])
    dst_suffix = bytearray(template[5])
    rv = b58encode_chk(prefix + payload + suffix)
    return rv, dst_prefix + payload + dst_suffix

def gen_valid_vectors():
    '''Generate valid test vectors'''
    glist = [gen_valid_base58_vector]
    tlist = [templates]
    while True:
        for template, valid_vector_generator in [(t, g) for g, l in zip(glist, tlist) for t in l]:
            rv, payload = valid_vector_generator(template)
            assert is_valid(rv)
            metadata = {x: y for x, y in zip(metadata_keys,template[3]) if y is not None}
            hexrepr = payload.hex()
            yield (rv, hexrepr, metadata)

def gen_invalid_base58_vector(template):
    '''Generate possibly invalid vector'''
    # kinds of invalid vectors:
    #   invalid prefix
    #   invalid payload length
    #   invalid (randomized) suffix (add random data)
    #   corrupt checksum
    corrupt_prefix = randbool(0.2)
    randomize_payload_size = randbool(0.2)
    corrupt_suffix = randbool(0.2)

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

    val = b58encode_chk(prefix + payload + suffix)
    if random.randint(0,10)<1: # line corruption
        if randbool(): # add random character to end
            val += random.choice(b58chars)
        else: # replace random character in the middle
            n = random.randint(0, len(val))
            val = val[0:n] + random.choice(b58chars) + val[n+1:]

    return val

def randbool(p = 0.5):
    '''Return True with P(p)'''
    return random.random() < p

def gen_invalid_vectors():
    '''Generate invalid test vectors'''
    # start with some manual edge-cases
    yield "",
    yield "x",
    glist = [gen_invalid_base58_vector]
    tlist = [templates]
    while True:
        for template, invalid_vector_generator in [(t, g) for g, l in zip(glist, tlist) for t in l]:
            val = invalid_vector_generator(template)
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

