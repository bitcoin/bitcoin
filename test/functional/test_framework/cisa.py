#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers for BIP460 cross-input signature aggregation (CISA) tests.

Implements the witness v2 keypath signature message, half-aggregation of
BIP340 signatures (BIP458), and the DahLIAS interactive full-aggregation
signing protocol (BIP459) run locally for all signers.
"""

import random

from .crypto.secp256k1 import G, GE
from .key import TaggedHash
from .script import TaprootSignatureMsg

CISA_MARKER_HALFAGG = 0xbc
CISA_MARKER_FULLAGG = 0xbd

CISA_SIGHASH_EPOCH = 0x01


def cisa_signature_hash(tx, spent_utxos, input_index, hash_type, agg_mode, annex=None):
    """Compute the BIP460 aggregated witness v2 keypath signature message:
    hash_TapSighash(0x01 || agg_mode || SigMsg(hash_type, 0))."""
    msg = TaprootSignatureMsg(tx, spent_utxos, hash_type, input_index, annex=annex)
    # TaprootSignatureMsg returns the BIP341 message including its leading
    # 0x00 epoch byte, which is replaced by the epoch 0x01 form here.
    return TaggedHash("TapSighash", bytes([CISA_SIGHASH_EPOCH, agg_mode]) + msg[1:])


def cisa_witness_element(sig_data=b"", hash_type=0, marker=None):
    """Construct an aggregated witness v2 keypath witness element from the
    input's signature data, a sighash byte for non-default sighash types and
    the marker on the final input of a group."""
    elem = sig_data
    if hash_type != 0:
        elem += bytes([hash_type])
    if marker is not None:
        elem += bytes([marker])
    return elem


def halfagg_aggregate(pms):
    """Half-aggregate (BIP458) a list of (pubkey32, msg32, sig64) triples."""
    assert len(pms) > 0
    r_values = []
    randomizer_input = b""
    s = 0
    for i, (pk, msg, sig) in enumerate(pms):
        assert len(pk) == 32 and len(msg) == 32 and len(sig) == 64
        r_values.append(sig[:32])
        randomizer_input += sig[:32] + pk + msg
        if i == 0:
            z = 1
        else:
            z = int.from_bytes(TaggedHash("HalfAgg/randomizer", randomizer_input), "big") % GE.ORDER
        s = (s + z * int.from_bytes(sig[32:], "big")) % GE.ORDER
    return b"".join(r_values) + s.to_bytes(32, "big")


def fullagg_sign(seckeys, msgs):
    """Produce a BIP459 (DahLIAS) aggregate signature by running the
    two-round interactive signing protocol locally for all signers.

    seckeys are the 32-byte (taproot tweaked) secret keys whose x-only public
    keys are the witness programs of the aggregated inputs, msgs are the
    corresponding 32-byte signature messages."""
    u = len(seckeys)
    assert u == len(msgs) > 0
    d = [int.from_bytes(sk, "big") for sk in seckeys]
    points = [dk * G for dk in d]
    pks = [P.to_bytes_xonly() for P in points]

    # Round 1: every signer generates a nonce pair (r1, r2) and shares the
    # public nonces. Randomness comes from the test framework's seeded PRNG.
    r1 = [random.randrange(1, GE.ORDER) for _ in range(u)]
    r2 = [random.randrange(1, GE.ORDER) for _ in range(u)]
    R2 = [k * G for k in r2]

    # Nonce aggregation and session values.
    R1_agg = (sum(r1) % GE.ORDER) * G
    R2_agg = (sum(r2) % GE.ORDER) * G
    assert not R1_agg.infinity and not R2_agg.infinity
    nonce_data = R1_agg.to_bytes_compressed() + R2_agg.to_bytes_compressed()
    for i in range(u):
        nonce_data += pks[i] + msgs[i] + R2[i].to_bytes_compressed()
    b = int.from_bytes(TaggedHash("FullAgg/noncecoef", nonce_data), "big") % GE.ORDER
    R = R1_agg + b * R2_agg
    assert not R.infinity

    e = 1 if R.y.is_even() else GE.ORDER - 1
    L = b""
    for i in range(u):
        L += pks[i] + msgs[i]

    # Round 2: every signer creates a partial signature, the sum of which is
    # the s part of the aggregate signature.
    s = 0
    for i in range(u):
        c_i = int.from_bytes(TaggedHash("FullAgg/sig", L + R.to_bytes_xonly() + pks[i] + msgs[i]), "big") % GE.ORDER
        d_prime = d[i] if points[i].y.is_even() else GE.ORDER - d[i]
        s = (s + e * (r1[i] + b * r2[i]) + c_i * d_prime) % GE.ORDER

    return R.to_bytes_xonly() + s.to_bytes(32, "big")
