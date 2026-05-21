#!/usr/bin/env python3
# Copyright (c) 2024 Random "Randy" Lattice and Sean Andersen
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.
'''
Generate a C file with ECDH testvectors from the Wycheproof project.
'''

import json
import sys

from binascii import hexlify, unhexlify
from wycheproof_utils import to_c_array

def should_skip_flags(test_vector_flags):
    # skip these vectors because they are for ASN.1 encoding issues and other curves.
    # for more details, see https://github.com/bitcoin-core/secp256k1/pull/1492#discussion_r1572491546
    flags_to_skip = {"InvalidAsn", "WrongCurve"}
    return any(flag in test_vector_flags for flag in flags_to_skip)

def should_skip_tcid(test_vector_tcid):
    # We skip some test case IDs that have a public key whose custom ASN.1 representation explicitly
    # encodes some curve parameters that are invalid. libsecp256k1 never parses this part so we do
    # not care testing those. See https://github.com/bitcoin-core/secp256k1/pull/1492#discussion_r1572491546
    tcids_to_skip = [496, 497, 502, 503, 504, 505, 507]
    return test_vector_tcid in tcids_to_skip

# Rudimentary ASN.1 DER public key parser.
# This should not be used for anything other than parsing Wycheproof test vectors.
def parse_der_pk(s):
    tag = s[0]
    L = int(s[1])
    offset = 0
    if L & 0x80:
        if L == 0x81:
            L = int(s[2])
            offset = 1
        elif L == 0x82:
            L = 256 * int(s[2]) + int(s[3])
            offset = 2
        else:
            raise ValueError("invalid L")
    value = s[(offset + 2):(L + 2 + offset)]
    rest = s[(L + 2 + offset):]

    if len(rest) > 0 or tag == 0x06: # OBJECT IDENTIFIER
        return parse_der_pk(rest)
    if tag == 0x03: # BIT STRING
        return value
    if tag == 0x30: # SEQUENCE
        return parse_der_pk(value)
    raise ValueError("unknown tag")

def parse_public_key(pk):
    der_pub_key = parse_der_pk(unhexlify(pk))   # Convert back to str and strip off the `0x`
    return hexlify(der_pub_key).decode()[2:]

def normalize_private_key(sk):
    # Ensure the private key is at most 64 characters long, retaining the last 64 if longer.
    # In the wycheproof test vectors, some private keys have leading zeroes
    normalized = sk[-64:].zfill(64)
    if len(normalized) != 64:
        raise ValueError("private key must be exactly 64 characters long.")
    return normalized

def normalize_expected_result(er):
    result_mapping = {"invalid": 0, "valid": 1, "acceptable": 1}
    return result_mapping[er]

filename_input = sys.argv[1]

with open(filename_input) as f:
    doc = json.load(f)

num_vectors = 0
offset_sk_running, offset_pk_running, offset_shared = 0, 0, 0
test_vectors_out = ""
private_keys = ""
shared_secrets = ""
public_keys = ""
cache_sks = {}
cache_public_keys = {}

for group in doc['testGroups']:
    assert group["type"] == "EcdhTest"
    assert group["curve"] == "secp256k1"
    for test_vector in group['tests']:
        if should_skip_flags(test_vector['flags']) or should_skip_tcid(test_vector['tcId']):
            continue

        public_key = parse_public_key(test_vector['public'])
        private_key = normalize_private_key(test_vector['private'])
        expected_result = normalize_expected_result(test_vector['result'])

        # // 2 to convert hex to byte length
        shared_size = len(test_vector['shared']) // 2
        sk_size = len(private_key) // 2
        pk_size = len(public_key) // 2

        new_sk = False
        sk = to_c_array(private_key)
        sk_offset = offset_sk_running

        # check for repeated sk
        if sk not in cache_sks:
            if num_vectors != 0 and sk_size != 0:
                private_keys += ",\n  "
            cache_sks[sk] = offset_sk_running
            private_keys += sk
            new_sk = True
        else:
            sk_offset = cache_sks[sk]

        new_pk = False
        pk = to_c_array(public_key) if public_key != '0x' else ''

        pk_offset = offset_pk_running
        # check for repeated pk
        if pk not in cache_public_keys:
            if num_vectors != 0 and len(pk) != 0:
                public_keys += ",\n  "
            cache_public_keys[pk] = offset_pk_running
            public_keys += pk
            new_pk = True
        else:
            pk_offset = cache_public_keys[pk]


        shared_secrets += ",\n  " if num_vectors and shared_size else ""
        shared_secrets += to_c_array(test_vector['shared'])
        wycheproof_tcid = test_vector['tcId']

        test_vectors_out += "  /" + "* tcId: " + str(test_vector['tcId']) + ". " + test_vector['comment'] + " *" + "/\n"
        test_vectors_out += f"  {{{pk_offset}, {pk_size}, {sk_offset}, {sk_size}, {offset_shared}, {shared_size}, {expected_result}, {wycheproof_tcid} }},\n"
        if new_sk:
            offset_sk_running += sk_size
        if new_pk:
            offset_pk_running += pk_size
        offset_shared += shared_size
        num_vectors += 1

struct_definition = """
typedef struct {
    size_t pk_offset;
    size_t pk_len;
    size_t sk_offset;
    size_t sk_len;
    size_t shared_offset;
    size_t shared_len;
    int expected_result;
    int wycheproof_tcid;
} wycheproof_ecdh_testvector;
"""

print("/* Note: this file was autogenerated using tests_wycheproof_ecdh.py. Do not edit. */")
print(f"#define SECP256K1_ECDH_WYCHEPROOF_NUMBER_TESTVECTORS ({num_vectors})")

print(struct_definition)

print("static const unsigned char wycheproof_ecdh_private_keys[]    = { " + private_keys + "};\n")
print("static const unsigned char wycheproof_ecdh_public_keys[] = { " + public_keys + "};\n")
print("static const unsigned char wycheproof_ecdh_shared_secrets[]  = { " + shared_secrets + "};\n")

print("static const wycheproof_ecdh_testvector testvectors[SECP256K1_ECDH_WYCHEPROOF_NUMBER_TESTVECTORS] = {")
print(test_vectors_out)
print("};")
