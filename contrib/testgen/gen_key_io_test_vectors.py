#!/usr/bin/env python3
# Copyright (c) 2012-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Generate valid and invalid base58/bech32(m) address and private key test vectors.

Usage:
    PYTHONPATH=../../test/functional/test_framework ./gen_key_io_test_vectors.py valid 70 > ../../src/test/data/key_io_valid.json
    PYTHONPATH=../../test/functional/test_framework ./gen_key_io_test_vectors.py invalid 70 > ../../src/test/data/key_io_invalid.json
'''
# 2012 Wladimir J. van der Laan
# Released under MIT License
import os
from itertools import islice
from base58 import b58encode_chk, b58decode_chk, b58chars
import random
from segwit_addr import bech32_encode, decode_segwit_address, convertbits, CHARSET, Encoding

# key types
PUBKEY_ADDRESS = 0
SCRIPT_ADDRESS = 5
PUBKEY_ADDRESS_TEST = 111
SCRIPT_ADDRESS_TEST = 196
PUBKEY_ADDRESS_REGTEST = 111
SCRIPT_ADDRESS_REGTEST = 196
PRIVKEY = 128
PRIVKEY_TEST = 239
PRIVKEY_REGTEST = 239

# script
OP_0 = 0x00
OP_1 = 0x51
OP_2 = 0x52
OP_3 = 0x53
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
p2wpkh_prefix = (OP_0, 20)
p2wsh_prefix = (OP_0, 32)
p2tr_prefix = (OP_1, 32)

metadata_keys = ['isPrivkey', 'chain', 'isCompressed', 'tryCaseFlip']
# templates for valid sequences
templates = [
  # prefix, payload_size, suffix, metadata, output_prefix, output_suffix
  #                                  None = N/A
  ((PUBKEY_ADDRESS,),         20, (),   (False, 'main',    None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS,),         20, (),   (False, 'main',    None,  None), script_prefix, script_suffix),
  ((PUBKEY_ADDRESS_TEST,),    20, (),   (False, 'test',    None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS_TEST,),    20, (),   (False, 'test',    None,  None), script_prefix, script_suffix),
  ((PUBKEY_ADDRESS_TEST,),    20, (),   (False, 'signet',  None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS_TEST,),    20, (),   (False, 'signet',  None,  None), script_prefix, script_suffix),
  ((PUBKEY_ADDRESS_REGTEST,), 20, (),   (False, 'regtest', None,  None), pubkey_prefix, pubkey_suffix),
  ((SCRIPT_ADDRESS_REGTEST,), 20, (),   (False, 'regtest', None,  None), script_prefix, script_suffix),
  ((PRIVKEY,),                32, (),   (True,  'main',    False, None), (),            ()),
  ((PRIVKEY,),                32, (1,), (True,  'main',    True,  None), (),            ()),
  ((PRIVKEY_TEST,),           32, (),   (True,  'test',    False, None), (),            ()),
  ((PRIVKEY_TEST,),           32, (1,), (True,  'test',    True,  None), (),            ()),
  ((PRIVKEY_TEST,),           32, (),   (True,  'signet',  False, None), (),            ()),
  ((PRIVKEY_TEST,),           32, (1,), (True,  'signet',  True,  None), (),            ()),
  ((PRIVKEY_REGTEST,),        32, (),   (True,  'regtest', False, None), (),            ()),
  ((PRIVKEY_REGTEST,),        32, (1,), (True,  'regtest', True,  None), (),            ())
]
# templates for valid bech32 sequences
bech32_templates = [
  # hrp, version, witprog_size, metadata, encoding, output_prefix
  ('bc',    0, 20, (False, 'main',    None, True), Encoding.BECH32,  p2wpkh_prefix),
  ('bc',    0, 32, (False, 'main',    None, True), Encoding.BECH32,  p2wsh_prefix),
  ('bc',    1, 32, (False, 'main',    None, True), Encoding.BECH32M, p2tr_prefix),
  ('bc',    2,  2, (False, 'main',    None, True), Encoding.BECH32M, (OP_2, 2)),
  ('tb',    0, 20, (False, 'test',    None, True), Encoding.BECH32,  p2wpkh_prefix),
  ('tb',    0, 32, (False, 'test',    None, True), Encoding.BECH32,  p2wsh_prefix),
  ('tb',    1, 32, (False, 'test',    None, True), Encoding.BECH32M, p2tr_prefix),
  ('tb',    3, 16, (False, 'test',    None, True), Encoding.BECH32M, (OP_3, 16)),
  ('tb',    0, 20, (False, 'signet',  None, True), Encoding.BECH32,  p2wpkh_prefix),
  ('tb',    0, 32, (False, 'signet',  None, True), Encoding.BECH32,  p2wsh_prefix),
  ('tb',    1, 32, (False, 'signet',  None, True), Encoding.BECH32M, p2tr_prefix),
  ('tb',    3, 32, (False, 'signet',  None, True), Encoding.BECH32M, (OP_3, 32)),
  ('bcrt',  0, 20, (False, 'regtest', None, True), Encoding.BECH32,  p2wpkh_prefix),
  ('bcrt',  0, 32, (False, 'regtest', None, True), Encoding.BECH32,  p2wsh_prefix),
  ('bcrt',  1, 32, (False, 'regtest', None, True), Encoding.BECH32M, p2tr_prefix),
  ('bcrt', 16, 40, (False, 'regtest', None, True), Encoding.BECH32M, (OP_16, 40))
]
# templates for invalid bech32 sequences
bech32_ng_templates = [
  # hrp, version, witprog_size, encoding, invalid_bech32, invalid_checksum, invalid_char
  ('tc',    0, 20, Encoding.BECH32,  False, False, False),
  ('bt',    1, 32, Encoding.BECH32M, False, False, False),
  ('tb',   17, 32, Encoding.BECH32M, False, False, False),
  ('bcrt',  3,  1, Encoding.BECH32M, False, False, False),
  ('bc',   15, 41, Encoding.BECH32M, False, False, False),
  ('tb',    0, 16, Encoding.BECH32,  False, False, False),
  ('bcrt',  0, 32, Encoding.BECH32,  True,  False, False),
  ('bc',    0, 16, Encoding.BECH32,  True,  False, False),
  ('tb',    0, 32, Encoding.BECH32,  False, True,  False),
  ('bcrt',  0, 20, Encoding.BECH32,  False, False, True),
  ('bc',    0, 20, Encoding.BECH32M, False, False, False),
  ('tb',    0, 32, Encoding.BECH32M, False, False, False),
  ('bcrt',  0, 20, Encoding.BECH32M, False, False, False),
  ('bc',    1, 32, Encoding.BECH32,  False, False, False),
  ('tb',    2, 16, Encoding.BECH32,  False, False, False),
  ('bcrt', 16, 20, Encoding.BECH32,  False, False, False),
]

def is_valid(v):
    '''Check vector v for validity'''
    if len(set(v) - set(b58chars)) > 0:
        return is_valid_bech32(v)
    result = b58decode_chk(v)
    if result is None:
        return is_valid_bech32(v)
    for template in templates:
        prefix = bytearray(template[0])
        suffix = bytearray(template[2])
        if result.startswith(prefix) and result.endswith(suffix):
            if (len(result) - len(prefix) - len(suffix)) == template[1]:
                return True
    return is_valid_bech32(v)

def is_valid_bech32(v):
    '''Check vector v for bech32 validity'''
    for hrp in ['bc', 'tb', 'bcrt']:
        if decode_segwit_address(hrp, v) != (None, None):
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

def gen_valid_bech32_vector(template):
    '''Generate valid bech32 vector'''
    hrp = template[0]
    witver = template[1]
    witprog = bytearray(os.urandom(template[2]))
    encoding = template[4]
    dst_prefix = bytearray(template[5])
    rv = bech32_encode(encoding, hrp, [witver] + convertbits(witprog, 8, 5))
    return rv, dst_prefix + witprog

def gen_valid_vectors():
    '''Generate valid test vectors'''
    glist = [gen_valid_base58_vector, gen_valid_bech32_vector]
    tlist = [templates, bech32_templates]
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

def gen_invalid_bech32_vector(template):
    '''Generate possibly invalid bech32 vector'''
    no_data = randbool(0.1)
    to_upper = randbool(0.1)
    hrp = template[0]
    witver = template[1]
    witprog = bytearray(os.urandom(template[2]))
    encoding = template[3]

    if no_data:
        rv = bech32_encode(encoding, hrp, [])
    else:
        data = [witver] + convertbits(witprog, 8, 5)
        if template[4] and not no_data:
            if template[2] % 5 in {2, 4}:
                data[-1] |= 1
            else:
                data.append(0)
        rv = bech32_encode(encoding, hrp, data)

    if template[5]:
        i = len(rv) - random.randrange(1, 7)
        rv = rv[:i] + random.choice(CHARSET.replace(rv[i], '')) + rv[i + 1:]
    if template[6]:
        i = len(hrp) + 1 + random.randrange(0, len(rv) - len(hrp) - 4)
        rv = rv[:i] + rv[i:i + 4].upper() + rv[i + 4:]

    if to_upper:
        rv = rv.swapcase()

    return rv

def randbool(p = 0.5):
    '''Return True with P(p)'''
    return random.random() < p

def gen_invalid_vectors():
    '''Generate invalid test vectors'''
    # start with some manual edge-cases
    yield "",
    yield "x",
    glist = [gen_invalid_base58_vector, gen_invalid_bech32_vector]
    tlist = [templates, bech32_ng_templates]
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

