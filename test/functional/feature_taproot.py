#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test Taproot softfork (BIPs 340-342)

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    SEQUENCE_FINAL,
)
from test_framework.script import (
    ANNEX_TAG,
    BIP341_sha_amounts,
    BIP341_sha_outputs,
    BIP341_sha_prevouts,
    BIP341_sha_scriptpubkeys,
    BIP341_sha_sequences,
    CScript,
    CScriptNum,
    CScriptOp,
    LEAF_VERSION_TAPSCRIPT,
    MAX_SCRIPT_ELEMENT_SIZE,
    OP_0,
    OP_1,
    OP_2,
    OP_3,
    OP_4,
    OP_5,
    OP_6,
    OP_7,
    OP_8,
    OP_9,
    OP_10,
    OP_11,
    OP_12,
    OP_16,
    OP_2DROP,
    OP_2DUP,
    OP_CHECKMULTISIG,
    OP_CHECKMULTISIGVERIFY,
    OP_CHECKSIG,
    OP_CHECKSIGADD,
    OP_CHECKSIGVERIFY,
    OP_CODESEPARATOR,
    OP_DROP,
    OP_DUP,
    OP_ELSE,
    OP_ENDIF,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_IF,
    OP_NOP,
    OP_NOT,
    OP_NOTIF,
    OP_PUSHDATA1,
    OP_RETURN,
    OP_SWAP,
    OP_TUCK,
    OP_VERIFY,
    SIGHASH_DEFAULT,
    SIGHASH_ALL,
    SIGHASH_NONE,
    SIGHASH_SINGLE,
    SIGHASH_ANYONECANPAY,
    is_op_success,
    taproot_construct,
)
from test_framework.script_util import (
    key_to_p2pk_script,
    key_to_p2pkh_script,
    key_to_p2wpkh_script,
)
from test_framework.spenders import (
    DEFAULT_CONTEXT,
    ERR_BAD_OPCODE,
    ERR_CHECKSIGVERIFY,
    ERR_CLEANSTACK,
    ERR_EVAL_FALSE,
    ERR_INVALID_STACK_OPERATION,
    ERR_OP_RETURN,
    ERR_PUSH_SIZE,
    ERR_SCHNORR_SIG,
    ERR_SCHNORR_SIG_HASHTYPE,
    ERR_SCHNORR_SIG_SIZE,
    ERR_SCRIPT_NUM,
    ERR_STACK_SIZE,
    ERR_TAPROOT_WRONG_CONTROL_SIZE,
    ERR_TAPSCRIPT_CHECKMULTISIG,
    ERR_TAPSCRIPT_EMPTY_PUBKEY,
    ERR_TAPSCRIPT_MINIMALIF,
    ERR_TAPSCRIPT_VALIDATION_WEIGHT,
    ERR_WITNESS_PROGRAM_MISMATCH,
    ERR_WITNESS_PROGRAM_WITNESS_EMPTY,
    SpenderTestMixin,
    add_spender,
    deep_eval,
    default_controlblock,
    default_key_tweaked,
    default_merklebranch,
    default_negflag,
    default_pubkey_internal,
    default_sighash,
    default_sign,
    default_signature,
    flatten,
    get,
    getter,
    override,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
)
from test_framework.wallet import NodeSigner
from test_framework.wallet_util import generate_keypair
from test_framework.key import (
    generate_privkey,
    compute_xonly_pubkey,
    ECKey,
)
from test_framework.crypto import secp256k1
from test_framework.address import (
    program_to_witness,
)
import json
import hashlib
import random

# Whether or not to output generated test vectors, in JSON format.
GEN_TEST_VECTORS = False

# === Helpers for the test ===

def random_checksig_style(pubkey):
    """Creates a random CHECKSIG* tapscript that would succeed with only the valid signature on witness stack."""
    opcode = random.choice([OP_CHECKSIG, OP_CHECKSIGVERIFY, OP_CHECKSIGADD])
    if opcode == OP_CHECKSIGVERIFY:
        ret = CScript([pubkey, opcode, OP_1])
    elif opcode == OP_CHECKSIGADD:
        num = random.choice([0, 0x7fffffff, -0x7fffffff])
        ret = CScript([num, pubkey, opcode, num + 1, OP_EQUAL])
    else:
        ret = CScript([pubkey, opcode])
    return bytes(ret)

def bitflipper(expr):
    """Return a callable that evaluates expr and returns it with a random bitflip."""
    def fn(ctx):
        sub = deep_eval(ctx, expr)
        assert isinstance(sub, bytes)
        return (int.from_bytes(sub, 'little') ^ (1 << random.randrange(len(sub) * 8))).to_bytes(len(sub), 'little')
    return fn

def zero_appender(expr):
    """Return a callable that evaluates expr and returns it with a zero added."""
    return lambda ctx: deep_eval(ctx, expr) + b"\x00"

def byte_popper(expr):
    """Return a callable that evaluates expr and returns it with its last byte removed."""
    return lambda ctx: deep_eval(ctx, expr)[:-1]


VALID_SIGHASHES_ECDSA = [
    SIGHASH_ALL,
    SIGHASH_NONE,
    SIGHASH_SINGLE,
    SIGHASH_ANYONECANPAY + SIGHASH_ALL,
    SIGHASH_ANYONECANPAY + SIGHASH_NONE,
    SIGHASH_ANYONECANPAY + SIGHASH_SINGLE
]

VALID_SIGHASHES_TAPROOT = [SIGHASH_DEFAULT] + VALID_SIGHASHES_ECDSA

VALID_SIGHASHES_TAPROOT_SINGLE = [
    SIGHASH_SINGLE,
    SIGHASH_ANYONECANPAY + SIGHASH_SINGLE
]

VALID_SIGHASHES_TAPROOT_NO_SINGLE = [h for h in VALID_SIGHASHES_TAPROOT if h not in VALID_SIGHASHES_TAPROOT_SINGLE]

SIGHASH_BITFLIP = {"failure": {"sighash": bitflipper(default_sighash)}}
SIG_POP_BYTE = {"failure": {"sign": byte_popper(default_sign)}}
SINGLE_SIG = {"inputs": [getter("sign")]}
SIG_ADD_ZERO = {"failure": {"sign": zero_appender(default_sign)}}

# === Actual test cases ===


def spenders_taproot_active():
    """Return a list of Spenders for testing post-Taproot activation behavior."""

    secs = [generate_privkey() for _ in range(8)]
    pubs = [compute_xonly_pubkey(sec)[0] for sec in secs]

    spenders = []

    # == Tests for BIP340 signature validation. ==
    # These are primarily tested through the test vectors implemented in libsecp256k1, and in src/tests/key_tests.cpp.
    # Some things are tested programmatically as well here.

    tap = taproot_construct(pubs[0])
    # Test with key with bit flipped.
    add_spender(spenders, "sig/key", tap=tap, key=secs[0], failure={"key_tweaked": bitflipper(default_key_tweaked)}, **ERR_SCHNORR_SIG)
    # Test with sighash with bit flipped.
    add_spender(spenders, "sig/sighash", tap=tap, key=secs[0], failure={"sighash": bitflipper(default_sighash)}, **ERR_SCHNORR_SIG)
    # Test with invalid R sign.
    add_spender(spenders, "sig/flip_r", tap=tap, key=secs[0], failure={"flag_flip_r": True}, **ERR_SCHNORR_SIG)
    # Test with invalid P sign.
    add_spender(spenders, "sig/flip_p", tap=tap, key=secs[0], failure={"flag_flip_p": True}, **ERR_SCHNORR_SIG)
    # Test with signature with bit flipped.
    add_spender(spenders, "sig/bitflip", tap=tap, key=secs[0], failure={"signature": bitflipper(default_signature)}, **ERR_SCHNORR_SIG)

    # == Test involving an internal public key not on the curve ==

    # X-only public keys are 32 bytes, but not every 32-byte array is a valid public key; only
    # around 50% of them are. This does not affect users using correct software; these "keys" have
    # no corresponding private key, and thus will never appear as output of key
    # generation/derivation/tweaking.
    #
    # Using an invalid public key as P2TR output key makes the UTXO unspendable. Revealing an
    # invalid public key as internal key in a P2TR script path spend also makes the spend invalid.
    # These conditions are explicitly spelled out in BIP341.
    #
    # It is however hard to create test vectors for this, because it involves "guessing" how a
    # hypothetical incorrect implementation deals with an obviously-invalid condition, and making
    # sure that guessed behavior (accepting it in certain condition) doesn't occur.
    #
    # The test case added here tries to detect a very specific bug a verifier could have: if they
    # don't verify whether or not a revealed internal public key in a script path spend is valid,
    # and (correctly) implement output_key == tweak(internal_key, tweakval) but (incorrectly) treat
    # tweak(invalid_key, tweakval) as equal the public key corresponding to private key tweakval.
    # This may seem like a far-fetched edge condition to test for, but in fact, the BIP341 wallet
    # pseudocode did exactly that (but obviously only triggerable by someone invoking the tweaking
    # function with an invalid public key, which shouldn't happen).

    # Generate an invalid public key
    while True:
        invalid_pub = random.randbytes(32)
        if not secp256k1.GE.is_valid_x(int.from_bytes(invalid_pub, 'big')):
            break

    # Implement a test case that detects validation logic which maps invalid public keys to the
    # point at infinity in the tweaking logic.
    tap = taproot_construct(invalid_pub, [("true", CScript([OP_1]))], treat_internal_as_infinity=True)
    add_spender(spenders, "output/invalid_x", tap=tap, key_tweaked=tap.tweak, failure={"leaf": "true", "inputs": []}, **ERR_WITNESS_PROGRAM_MISMATCH)

    # Do the same thing without invalid point, to make sure there is no mistake in the test logic.
    tap = taproot_construct(pubs[0], [("true", CScript([OP_1]))])
    add_spender(spenders, "output/invalid_x_mock", tap=tap, key=secs[0], leaf="true", inputs=[])

    # == Tests for signature hashing ==

    # Run all tests once with no annex, and once with a valid random annex.
    for annex in [None, lambda _: bytes([ANNEX_TAG]) + random.randbytes(random.randrange(0, 250))]:
        # Non-empty annex is non-standard
        no_annex = annex is None

        # Sighash mutation tests (test all sighash combinations)
        for hashtype in VALID_SIGHASHES_TAPROOT:
            common = {"annex": annex, "hashtype": hashtype, "standard": no_annex}

            # Pure pubkey
            tap = taproot_construct(pubs[0])
            add_spender(spenders, "sighash/purepk", tap=tap, key=secs[0], **common, **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)

            # Pubkey/P2PK script combination
            scripts = [("s0", CScript(random_checksig_style(pubs[1])))]
            tap = taproot_construct(pubs[0], scripts)
            add_spender(spenders, "sighash/keypath_hashtype_%x" % hashtype, tap=tap, key=secs[0], **common, **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)
            add_spender(spenders, "sighash/scriptpath_hashtype_%x" % hashtype, tap=tap, leaf="s0", key=secs[1], **common, **SINGLE_SIG, **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)

            # Test SIGHASH_SINGLE behavior in combination with mismatching outputs
            if hashtype in VALID_SIGHASHES_TAPROOT_SINGLE:
                add_spender(spenders, "sighash/keypath_hashtype_mis_%x" % hashtype, tap=tap, key=secs[0], annex=annex, standard=no_annex, hashtype_actual=random.choice(VALID_SIGHASHES_TAPROOT_NO_SINGLE), failure={"hashtype_actual": hashtype}, **ERR_SCHNORR_SIG_HASHTYPE, need_vin_vout_mismatch=True)
                add_spender(spenders, "sighash/scriptpath_hashtype_mis_%x" % hashtype, tap=tap, leaf="s0", key=secs[1], annex=annex, standard=no_annex, hashtype_actual=random.choice(VALID_SIGHASHES_TAPROOT_NO_SINGLE), **SINGLE_SIG, failure={"hashtype_actual": hashtype}, **ERR_SCHNORR_SIG_HASHTYPE, need_vin_vout_mismatch=True)

        # Test OP_CODESEPARATOR impact on sighashing.
        def hashtype(_):
            return random.choice(VALID_SIGHASHES_TAPROOT)

        common = {"annex": annex, "hashtype": hashtype, "standard": no_annex}
        scripts = [
            ("pk_codesep", CScript(random_checksig_style(pubs[1]) + bytes([OP_CODESEPARATOR]))),  # codesep after checksig
            ("codesep_pk", CScript(bytes([OP_CODESEPARATOR]) + random_checksig_style(pubs[1]))),  # codesep before checksig
            ("branched_codesep", CScript([random.randbytes(random.randrange(2, 511)), OP_DROP, OP_IF, OP_CODESEPARATOR, pubs[0], OP_ELSE, OP_CODESEPARATOR, pubs[1], OP_ENDIF, OP_CHECKSIG])),  # branch dependent codesep
            # Note that the first data push in the "branched_codesep" script has the purpose of
            # randomizing the sighash, both by varying script size and content. In order to
            # avoid MINIMALDATA script verification errors caused by not-minimal-encoded data
            # pushes (e.g. `OP_PUSH1 1` instead of `OP_1`), we set a minimum data size of 2 bytes.
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "sighash/pk_codesep", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)
        add_spender(spenders, "sighash/codesep_pk", tap=tap, leaf="codesep_pk", key=secs[1], codeseppos=0, **common, **SINGLE_SIG, **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)
        add_spender(spenders, "sighash/branched_codesep/left", tap=tap, leaf="branched_codesep", key=secs[0], codeseppos=3, **common, inputs=[getter("sign"), b'\x01'], **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)
        add_spender(spenders, "sighash/branched_codesep/right", tap=tap, leaf="branched_codesep", key=secs[1], codeseppos=6, **common, inputs=[getter("sign"), b''], **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)
        add_spender(spenders, "sighash/codesep_pk_wrongpos1", tap=tap, leaf="codesep_pk", key=secs[1], codeseppos=0, **common, **SINGLE_SIG, failure={"codeseppos": 1}, **ERR_SCHNORR_SIG)
        add_spender(spenders, "sighash/codesep_pk_wrongpos2", tap=tap, leaf="codesep_pk", key=secs[1], codeseppos=0, **common, **SINGLE_SIG, failure={"codeseppos": 0xfffffffe}, **ERR_SCHNORR_SIG)

    # Reusing the scripts above, test that various features affect the sighash.
    add_spender(spenders, "sighash/annex", tap=tap, leaf="pk_codesep", key=secs[1], hashtype=hashtype, standard=False, **SINGLE_SIG, annex=bytes([ANNEX_TAG]), failure={"sighash": override(default_sighash, annex=None)}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/script", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, failure={"sighash": override(default_sighash, script_taproot=tap.leaves["codesep_pk"].script)}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/leafver", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, failure={"sighash": override(default_sighash, leafversion=random.choice([x & 0xFE for x in range(0x100) if x & 0xFE != LEAF_VERSION_TAPSCRIPT]))}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, failure={"sighash": override(default_sighash, leaf=None)}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/keypath", tap=tap, key=secs[0], **common, failure={"sighash": override(default_sighash, leaf="pk_codesep")}, **ERR_SCHNORR_SIG)

    # Test that invalid hashtypes don't work, both in key path and script path spends
    def hashtype(_):
        return random.choice(VALID_SIGHASHES_TAPROOT)

    for invalid_hashtype in [x for x in range(0x100) if x not in VALID_SIGHASHES_TAPROOT]:
        add_spender(spenders, "sighash/keypath_unk_hashtype_%x" % invalid_hashtype, tap=tap, key=secs[0], hashtype=hashtype, failure={"hashtype": invalid_hashtype}, **ERR_SCHNORR_SIG_HASHTYPE)
        add_spender(spenders, "sighash/scriptpath_unk_hashtype_%x" % invalid_hashtype, tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=hashtype, failure={"hashtype": invalid_hashtype}, **ERR_SCHNORR_SIG_HASHTYPE)

    # Test that hashtype 0 cannot have a hashtype byte, and 1 must have one.
    add_spender(spenders, "sighash/hashtype0_byte_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_DEFAULT])}, **ERR_SCHNORR_SIG_HASHTYPE)
    add_spender(spenders, "sighash/hashtype0_byte_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_DEFAULT])}, **ERR_SCHNORR_SIG_HASHTYPE)
    add_spender(spenders, "sighash/hashtype1_byte_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/hashtype1_byte_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SCHNORR_SIG)
    # Test that hashtype 0 and hashtype 1 cannot be transmuted into each other.
    add_spender(spenders, "sighash/hashtype0to1_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_ALL])}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/hashtype0to1_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_ALL])}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/hashtype1to0_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "sighash/hashtype1to0_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SCHNORR_SIG)

    # Test aspects of signatures with unusual lengths
    for hashtype in [SIGHASH_DEFAULT, random.choice(VALID_SIGHASHES_TAPROOT)]:
        scripts = [
            ("csv", CScript([pubs[2], OP_CHECKSIGVERIFY, OP_1])),
            ("cs_pos", CScript([pubs[2], OP_CHECKSIG])),
            ("csa_pos", CScript([OP_0, pubs[2], OP_CHECKSIGADD, OP_1, OP_EQUAL])),
            ("cs_neg", CScript([pubs[2], OP_CHECKSIG, OP_NOT])),
            ("csa_neg", CScript([OP_2, pubs[2], OP_CHECKSIGADD, OP_2, OP_EQUAL]))
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[3], scripts)
        # Empty signatures
        add_spender(spenders, "siglen/empty_keypath", tap=tap, key=secs[3], hashtype=hashtype, failure={"sign": b""}, **ERR_SCHNORR_SIG_SIZE)
        add_spender(spenders, "siglen/empty_csv", tap=tap, key=secs[2], leaf="csv", hashtype=hashtype, **SINGLE_SIG, failure={"sign": b""}, **ERR_CHECKSIGVERIFY)
        add_spender(spenders, "siglen/empty_cs", tap=tap, key=secs[2], leaf="cs_pos", hashtype=hashtype, **SINGLE_SIG, failure={"sign": b""}, **ERR_EVAL_FALSE)
        add_spender(spenders, "siglen/empty_csa", tap=tap, key=secs[2], leaf="csa_pos", hashtype=hashtype, **SINGLE_SIG, failure={"sign": b""}, **ERR_EVAL_FALSE)
        add_spender(spenders, "siglen/empty_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": lambda _: random.randbytes(random.randrange(1, 63))}, **ERR_SCHNORR_SIG_SIZE)
        add_spender(spenders, "siglen/empty_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": lambda _: random.randbytes(random.randrange(66, 100))}, **ERR_SCHNORR_SIG_SIZE)
        # Appending a zero byte to signatures invalidates them
        add_spender(spenders, "siglen/padzero_keypath", tap=tap, key=secs[3], hashtype=hashtype, **SIG_ADD_ZERO, **(ERR_SCHNORR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_csv", tap=tap, key=secs[2], leaf="csv", hashtype=hashtype, **SINGLE_SIG, **SIG_ADD_ZERO, **(ERR_SCHNORR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_cs", tap=tap, key=secs[2], leaf="cs_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_ADD_ZERO, **(ERR_SCHNORR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_csa", tap=tap, key=secs[2], leaf="csa_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_ADD_ZERO, **(ERR_SCHNORR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_ADD_ZERO, **(ERR_SCHNORR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_ADD_ZERO, **(ERR_SCHNORR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG_SIZE))
        # Removing the last byte from signatures invalidates them
        add_spender(spenders, "siglen/popbyte_keypath", tap=tap, key=secs[3], hashtype=hashtype, **SIG_POP_BYTE, **(ERR_SCHNORR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG))
        add_spender(spenders, "siglen/popbyte_csv", tap=tap, key=secs[2], leaf="csv", hashtype=hashtype, **SINGLE_SIG, **SIG_POP_BYTE, **(ERR_SCHNORR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG))
        add_spender(spenders, "siglen/popbyte_cs", tap=tap, key=secs[2], leaf="cs_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_POP_BYTE, **(ERR_SCHNORR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG))
        add_spender(spenders, "siglen/popbyte_csa", tap=tap, key=secs[2], leaf="csa_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_POP_BYTE, **(ERR_SCHNORR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG))
        add_spender(spenders, "siglen/popbyte_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_POP_BYTE, **(ERR_SCHNORR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG))
        add_spender(spenders, "siglen/popbyte_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_POP_BYTE, **(ERR_SCHNORR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SCHNORR_SIG))
        # Verify that an invalid signature is not allowed, not even when the CHECKSIG* is expected to fail.
        add_spender(spenders, "siglen/invalid_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": default_sign, "sighash": bitflipper(default_sighash)}, **ERR_SCHNORR_SIG)
        add_spender(spenders, "siglen/invalid_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": default_sign, "sighash": bitflipper(default_sighash)}, **ERR_SCHNORR_SIG)

    # == Test that BIP341 spending only applies to witness version 1, program length 32, no P2SH ==

    for p2sh in [False, True]:
        for witver in range(1, 17):
            for witlen in [20, 31, 32, 33]:
                def mutate(spk):
                    prog = spk[2:]
                    assert_equal(len(prog), 32)
                    if witlen < 32:
                        prog = prog[0:witlen]
                    elif witlen > 32:
                        prog += bytes([0 for _ in range(witlen - 32)])
                    return CScript([CScriptOp.encode_op_n(witver), prog])
                scripts = [("s0", CScript([pubs[0], OP_CHECKSIG])), ("dummy", CScript([OP_RETURN]))]
                tap = taproot_construct(pubs[1], scripts)
                if not p2sh and witver == 1 and witlen == 32:
                    add_spender(spenders, "applic/keypath", p2sh=p2sh, spk_mutate_pre_p2sh=mutate, tap=tap, key=secs[1], **SIGHASH_BITFLIP, **ERR_SCHNORR_SIG)
                    add_spender(spenders, "applic/scriptpath", p2sh=p2sh, leaf="s0", spk_mutate_pre_p2sh=mutate, tap=tap, key=secs[0], **SINGLE_SIG, failure={"leaf": "dummy"}, **ERR_OP_RETURN)
                else:
                    add_spender(spenders, "applic/keypath", p2sh=p2sh, spk_mutate_pre_p2sh=mutate, tap=tap, key=secs[1], standard=False)
                    add_spender(spenders, "applic/scriptpath", p2sh=p2sh, leaf="s0", spk_mutate_pre_p2sh=mutate, tap=tap, key=secs[0], **SINGLE_SIG, standard=False)

    # == Test various aspects of BIP341 spending paths ==

    # A set of functions that compute the hashing partner in a Merkle tree, designed to exercise
    # edge cases. This relies on the taproot_construct feature that a lambda can be passed in
    # instead of a subtree, to compute the partner to be hashed with.
    PARTNER_MERKLE_FN = [
        # Combine with itself
        lambda h: h,
        # Combine with hash 0
        lambda h: bytes([0 for _ in range(32)]),
        # Combine with hash 2^256-1
        lambda h: bytes([0xff for _ in range(32)]),
        # Combine with itself-1 (BE)
        lambda h: (int.from_bytes(h, 'big') - 1).to_bytes(32, 'big'),
        # Combine with itself+1 (BE)
        lambda h: (int.from_bytes(h, 'big') + 1).to_bytes(32, 'big'),
        # Combine with itself-1 (LE)
        lambda h: (int.from_bytes(h, 'little') - 1).to_bytes(32, 'big'),
        # Combine with itself+1 (LE)
        lambda h: (int.from_bytes(h, 'little') + 1).to_bytes(32, 'little'),
        # Combine with random bitflipped version of self.
        lambda h: (int.from_bytes(h, 'little') ^ (1 << random.randrange(256))).to_bytes(32, 'little')
    ]
    # Start with a tree of that has depth 1 for "128deep" and depth 2 for "129deep".
    scripts = [("128deep", CScript([pubs[0], OP_CHECKSIG])), [("129deep", CScript([pubs[0], OP_CHECKSIG])), random.choice(PARTNER_MERKLE_FN)]]
    # Add 127 nodes on top of that tree, so that "128deep" and "129deep" end up at their designated depths.
    for _ in range(127):
        scripts = [scripts, random.choice(PARTNER_MERKLE_FN)]
    tap = taproot_construct(pubs[0], scripts)
    # Test that spends with a depth of 128 work, but 129 doesn't (even with a tree with weird Merkle branches in it).
    add_spender(spenders, "spendpath/merklelimit", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"leaf": "129deep"}, **ERR_TAPROOT_WRONG_CONTROL_SIZE)
    # Test that flipping the negation bit invalidates spends.
    add_spender(spenders, "spendpath/negflag", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"negflag": lambda ctx: 1 - default_negflag(ctx)}, **ERR_WITNESS_PROGRAM_MISMATCH)
    # Test that bitflips in the Merkle branch invalidate it.
    add_spender(spenders, "spendpath/bitflipmerkle", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"merklebranch": bitflipper(default_merklebranch)}, **ERR_WITNESS_PROGRAM_MISMATCH)
    # Test that bitflips in the internal pubkey invalidate it.
    add_spender(spenders, "spendpath/bitflippubkey", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"pubkey_internal": bitflipper(default_pubkey_internal)}, **ERR_WITNESS_PROGRAM_MISMATCH)
    # Test that empty witnesses are invalid.
    add_spender(spenders, "spendpath/emptywit", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"witness": []}, **ERR_WITNESS_PROGRAM_WITNESS_EMPTY)
    # Test that adding garbage to the control block invalidates it.
    add_spender(spenders, "spendpath/padlongcontrol", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_controlblock(ctx) + random.randbytes(random.randrange(1, 32))}, **ERR_TAPROOT_WRONG_CONTROL_SIZE)
    # Test that truncating the control block invalidates it.
    add_spender(spenders, "spendpath/trunclongcontrol", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_merklebranch(ctx)[0:random.randrange(1, 32)]}, **ERR_TAPROOT_WRONG_CONTROL_SIZE)

    scripts = [("s", CScript([pubs[0], OP_CHECKSIG]))]
    tap = taproot_construct(pubs[1], scripts)
    # Test that adding garbage to the control block invalidates it.
    add_spender(spenders, "spendpath/padshortcontrol", tap=tap, leaf="s", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_controlblock(ctx) + random.randbytes(random.randrange(1, 32))}, **ERR_TAPROOT_WRONG_CONTROL_SIZE)
    # Test that truncating the control block invalidates it.
    add_spender(spenders, "spendpath/truncshortcontrol", tap=tap, leaf="s", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_merklebranch(ctx)[0:random.randrange(1, 32)]}, **ERR_TAPROOT_WRONG_CONTROL_SIZE)
    # Test that truncating the control block to 1 byte ("-1 Merkle length") invalidates it
    add_spender(spenders, "spendpath/trunc1shortcontrol", tap=tap, leaf="s", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_merklebranch(ctx)[0:1]}, **ERR_TAPROOT_WRONG_CONTROL_SIZE)

    # == Test BIP342 edge cases ==

    csa_low_val = random.randrange(0, 17) # Within range for OP_n
    csa_low_result = csa_low_val + 1

    csa_high_val = random.randrange(17, 100) if random.getrandbits(1) else random.randrange(-100, -1) # Outside OP_n range
    csa_high_result = csa_high_val + 1

    OVERSIZE_NUMBER = 2**31
    assert_equal(len(CScriptNum.encode(CScriptNum(OVERSIZE_NUMBER))), 6)
    assert_equal(len(CScriptNum.encode(CScriptNum(OVERSIZE_NUMBER-1))), 5)

    big_choices = []
    big_scriptops = []
    for i in range(1000):
        r = random.randrange(len(pubs))
        big_choices.append(r)
        big_scriptops += [pubs[r], OP_CHECKSIGVERIFY]


    def big_spend_inputs(ctx):
        """Helper function to construct the script input for t33/t34 below."""
        # Instead of signing 999 times, precompute signatures for every (key, hashtype) combination
        sigs = {}
        for ht in VALID_SIGHASHES_TAPROOT:
            for k in range(len(pubs)):
                sigs[(k, ht)] = override(default_sign, hashtype=ht, key=secs[k])(ctx)
        num = get(ctx, "num")
        return [sigs[(big_choices[i], random.choice(VALID_SIGHASHES_TAPROOT))] for i in range(num - 1, -1, -1)]

    # Various BIP342 features
    scripts = [
        # 0) drop stack element and OP_CHECKSIG
        ("t0", CScript([OP_DROP, pubs[1], OP_CHECKSIG])),
        # 1) normal OP_CHECKSIG
        ("t1", CScript([pubs[1], OP_CHECKSIG])),
        # 2) normal OP_CHECKSIGVERIFY
        ("t2", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_1])),
        # 3) Hypothetical OP_CHECKMULTISIG script that takes a single sig as input
        ("t3", CScript([OP_0, OP_SWAP, OP_1, pubs[1], OP_1, OP_CHECKMULTISIG])),
        # 4) Hypothetical OP_CHECKMULTISIGVERIFY script that takes a single sig as input
        ("t4", CScript([OP_0, OP_SWAP, OP_1, pubs[1], OP_1, OP_CHECKMULTISIGVERIFY, OP_1])),
        # 5) OP_IF script that needs a true input
        ("t5", CScript([OP_IF, pubs[1], OP_CHECKSIG, OP_ELSE, OP_RETURN, OP_ENDIF])),
        # 6) OP_NOTIF script that needs a true input
        ("t6", CScript([OP_NOTIF, OP_RETURN, OP_ELSE, pubs[1], OP_CHECKSIG, OP_ENDIF])),
        # 7) OP_CHECKSIG with an empty key
        ("t7", CScript([OP_0, OP_CHECKSIG])),
        # 8) OP_CHECKSIGVERIFY with an empty key
        ("t8", CScript([OP_0, OP_CHECKSIGVERIFY, OP_1])),
        # 9) normal OP_CHECKSIGADD that also ensures return value is correct
        ("t9", CScript([csa_low_val, pubs[1], OP_CHECKSIGADD, csa_low_result, OP_EQUAL])),
        # 10) OP_CHECKSIGADD with empty key
        ("t10", CScript([csa_low_val, OP_0, OP_CHECKSIGADD, csa_low_result, OP_EQUAL])),
        # 11) OP_CHECKSIGADD with missing counter stack element
        ("t11", CScript([pubs[1], OP_CHECKSIGADD, OP_1, OP_EQUAL])),
        # 12) OP_CHECKSIG that needs invalid signature
        ("t12", CScript([pubs[1], OP_CHECKSIGVERIFY, pubs[0], OP_CHECKSIG, OP_NOT])),
        # 13) OP_CHECKSIG with empty key that needs invalid signature
        ("t13", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_0, OP_CHECKSIG, OP_NOT])),
        # 14) OP_CHECKSIGADD that needs invalid signature
        ("t14", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_0, pubs[0], OP_CHECKSIGADD, OP_NOT])),
        # 15) OP_CHECKSIGADD with empty key that needs invalid signature
        ("t15", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_0, OP_0, OP_CHECKSIGADD, OP_NOT])),
        # 16) OP_CHECKSIG with unknown pubkey type
        ("t16", CScript([OP_1, OP_CHECKSIG])),
        # 17) OP_CHECKSIGADD with unknown pubkey type
        ("t17", CScript([OP_0, OP_1, OP_CHECKSIGADD])),
        # 18) OP_CHECKSIGVERIFY with unknown pubkey type
        ("t18", CScript([OP_1, OP_CHECKSIGVERIFY, OP_1])),
        # 19) script longer than 10000 bytes and over 201 non-push opcodes
        ("t19", CScript([OP_0, OP_0, OP_2DROP] * 10001 + [pubs[1], OP_CHECKSIG])),
        # 20) OP_CHECKSIGVERIFY with empty key
        ("t20", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_0, OP_0, OP_CHECKSIGVERIFY, OP_1])),
        # 21) Script that grows the stack to 1000 elements
        ("t21", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_1] + [OP_DUP] * 999 + [OP_DROP] * 999)),
        # 22) Script that grows the stack to 1001 elements
        ("t22", CScript([pubs[1], OP_CHECKSIGVERIFY, OP_1] + [OP_DUP] * 1000 + [OP_DROP] * 1000)),
        # 23) Script that expects an input stack of 1000 elements
        ("t23", CScript([OP_DROP] * 999 + [pubs[1], OP_CHECKSIG])),
        # 24) Script that expects an input stack of 1001 elements
        ("t24", CScript([OP_DROP] * 1000 + [pubs[1], OP_CHECKSIG])),
        # 25) Script that pushes a MAX_SCRIPT_ELEMENT_SIZE-bytes element
        ("t25", CScript([random.randbytes(MAX_SCRIPT_ELEMENT_SIZE), OP_DROP, pubs[1], OP_CHECKSIG])),
        # 26) Script that pushes a (MAX_SCRIPT_ELEMENT_SIZE+1)-bytes element
        ("t26", CScript([random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP, pubs[1], OP_CHECKSIG])),
        # 27) CHECKSIGADD that must fail because numeric argument number is >4 bytes
        ("t27", CScript([CScriptNum(OVERSIZE_NUMBER), pubs[1], OP_CHECKSIGADD])),
        # 28) Pushes random CScriptNum value, checks OP_CHECKSIGADD result
        ("t28", CScript([csa_high_val, pubs[1], OP_CHECKSIGADD, csa_high_result, OP_EQUAL])),
        # 29) CHECKSIGADD that succeeds with proper sig because numeric argument number is <=4 bytes
        ("t29", CScript([CScriptNum(OVERSIZE_NUMBER-1), pubs[1], OP_CHECKSIGADD])),
        # 30) Variant of t1 with "normal" 33-byte pubkey
        ("t30", CScript([b'\x03' + pubs[1], OP_CHECKSIG])),
        # 31) Variant of t2 with "normal" 33-byte pubkey
        ("t31", CScript([b'\x02' + pubs[1], OP_CHECKSIGVERIFY, OP_1])),
        # 32) Variant of t28 with "normal" 33-byte pubkey
        ("t32", CScript([csa_high_val, b'\x03' + pubs[1], OP_CHECKSIGADD, csa_high_result, OP_EQUAL])),
        # 33) 999-of-999 multisig
        ("t33", CScript(big_scriptops[:1998] + [OP_1])),
        # 34) 1000-of-1000 multisig
        ("t34", CScript(big_scriptops[:2000] + [OP_1])),
        # 35) Variant of t9 that uses a non-minimally encoded input arg
        ("t35", CScript([bytes([csa_low_val]), pubs[1], OP_CHECKSIGADD, csa_low_result, OP_EQUAL])),
        # 36) Empty script
        ("t36", CScript([])),
    ]
    # Add many dummies to test huge trees
    for j in range(100000):
        scripts.append((None, CScript([OP_RETURN, random.randrange(100000)])))
    random.shuffle(scripts)
    tap = taproot_construct(pubs[0], scripts)
    common = {
        "hashtype": hashtype,
        "key": secs[1],
        "tap": tap,
    }
    # Test that MAX_SCRIPT_ELEMENT_SIZE byte stack element inputs are valid, but not one more (and 80 bytes is standard but 81 is not).
    add_spender(spenders, "tapscript/inputmaxlimit", leaf="t0", **common, standard=False, inputs=[getter("sign"), random.randbytes(MAX_SCRIPT_ELEMENT_SIZE)], failure={"inputs": [getter("sign"), random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1)]}, **ERR_PUSH_SIZE)
    add_spender(spenders, "tapscript/input80limit", leaf="t0", **common, inputs=[getter("sign"), random.randbytes(80)])
    add_spender(spenders, "tapscript/input81limit", leaf="t0", **common, standard=False, inputs=[getter("sign"), random.randbytes(81)])
    # Test that OP_CHECKMULTISIG and OP_CHECKMULTISIGVERIFY cause failure, but OP_CHECKSIG and OP_CHECKSIGVERIFY work.
    add_spender(spenders, "tapscript/disabled_checkmultisig", leaf="t1", **common, **SINGLE_SIG, failure={"leaf": "t3"}, **ERR_TAPSCRIPT_CHECKMULTISIG)
    add_spender(spenders, "tapscript/disabled_checkmultisigverify", leaf="t2", **common, **SINGLE_SIG, failure={"leaf": "t4"}, **ERR_TAPSCRIPT_CHECKMULTISIG)
    # Test that OP_IF and OP_NOTIF do not accept non-0x01 as truth value (the MINIMALIF rule is consensus in Tapscript)
    add_spender(spenders, "tapscript/minimalif", leaf="t5", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x02']}, **ERR_TAPSCRIPT_MINIMALIF)
    add_spender(spenders, "tapscript/minimalnotif", leaf="t6", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x03']}, **ERR_TAPSCRIPT_MINIMALIF)
    add_spender(spenders, "tapscript/minimalif", leaf="t5", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x0001']}, **ERR_TAPSCRIPT_MINIMALIF)
    add_spender(spenders, "tapscript/minimalnotif", leaf="t6", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x0100']}, **ERR_TAPSCRIPT_MINIMALIF)
    # Test that 1-byte public keys (which are unknown) are acceptable but nonstandard with unrelated signatures, but 0-byte public keys are not valid.
    add_spender(spenders, "tapscript/unkpk/checksig", leaf="t16", standard=False, **common, **SINGLE_SIG, failure={"leaf": "t7"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/unkpk/checksigadd", leaf="t17", standard=False, **common, **SINGLE_SIG, failure={"leaf": "t10"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/unkpk/checksigverify", leaf="t18", standard=False, **common, **SINGLE_SIG, failure={"leaf": "t8"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    # Test that 33-byte public keys (which are unknown) are acceptable but nonstandard with valid signatures, but normal pubkeys are not valid in that case.
    add_spender(spenders, "tapscript/oldpk/checksig", leaf="t30", standard=False, **common, **SINGLE_SIG, sighash=bitflipper(default_sighash), failure={"leaf": "t1"}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "tapscript/oldpk/checksigadd", leaf="t31", standard=False, **common, **SINGLE_SIG, sighash=bitflipper(default_sighash), failure={"leaf": "t2"}, **ERR_SCHNORR_SIG)
    add_spender(spenders, "tapscript/oldpk/checksigverify", leaf="t32", standard=False, **common, **SINGLE_SIG, sighash=bitflipper(default_sighash), failure={"leaf": "t28"}, **ERR_SCHNORR_SIG)
    # Test that 0-byte public keys are not acceptable.
    add_spender(spenders, "tapscript/emptypk/checksig", leaf="t1", **SINGLE_SIG, **common, failure={"leaf": "t7"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/emptypk/checksigverify", leaf="t2", **SINGLE_SIG, **common, failure={"leaf": "t8"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/emptypk/checksigadd", leaf="t9", **SINGLE_SIG, **common, failure={"leaf": "t10"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/emptypk/checksigadd", leaf="t35", standard=False, **SINGLE_SIG, **common, failure={"leaf": "t10"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    # Test that OP_CHECKSIGADD results are as expected
    add_spender(spenders, "tapscript/checksigaddresults", leaf="t28", **SINGLE_SIG, **common, failure={"leaf": "t27"}, **ERR_SCRIPT_NUM)
    add_spender(spenders, "tapscript/checksigaddoversize", leaf="t29", **SINGLE_SIG, **common, failure={"leaf": "t27"}, **ERR_SCRIPT_NUM)
    # Test that OP_CHECKSIGADD requires 3 stack elements.
    add_spender(spenders, "tapscript/checksigadd3args", leaf="t9", **SINGLE_SIG, **common, failure={"leaf": "t11"}, **ERR_INVALID_STACK_OPERATION)
    # Test that empty signatures do not cause script failure in OP_CHECKSIG and OP_CHECKSIGADD (but do fail with empty pubkey, and do fail OP_CHECKSIGVERIFY)
    add_spender(spenders, "tapscript/emptysigs/checksig", leaf="t12", **common, inputs=[b'', getter("sign")], failure={"leaf": "t13"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/emptysigs/nochecksigverify", leaf="t12", **common, inputs=[b'', getter("sign")], failure={"leaf": "t20"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    add_spender(spenders, "tapscript/emptysigs/checksigadd", leaf="t14", **common, inputs=[b'', getter("sign")], failure={"leaf": "t15"}, **ERR_TAPSCRIPT_EMPTY_PUBKEY)
    # Test that scripts over 10000 bytes (and over 201 non-push ops) are acceptable.
    add_spender(spenders, "tapscript/no10000limit", leaf="t19", **SINGLE_SIG, **common)
    # Test that a stack size of 1000 elements is permitted, but 1001 isn't.
    add_spender(spenders, "tapscript/1000stack", leaf="t21", **SINGLE_SIG, **common, failure={"leaf": "t22"}, **ERR_STACK_SIZE)
    # Test that an input stack size of 1000 elements is permitted, but 1001 isn't.
    add_spender(spenders, "tapscript/1000inputs", leaf="t23", **common, inputs=[getter("sign")] + [b'' for _ in range(999)], failure={"leaf": "t24", "inputs": [getter("sign")] + [b'' for _ in range(1000)]}, **ERR_STACK_SIZE)
    # Test that pushing a MAX_SCRIPT_ELEMENT_SIZE byte stack element is valid, but one longer is not.
    add_spender(spenders, "tapscript/pushmaxlimit", leaf="t25", **common, **SINGLE_SIG, failure={"leaf": "t26"}, **ERR_PUSH_SIZE)
    # Test that 999-of-999 multisig works (but 1000-of-1000 triggers stack size limits)
    add_spender(spenders, "tapscript/bigmulti", leaf="t33", **common, inputs=big_spend_inputs, num=999, failure={"leaf": "t34", "num": 1000}, **ERR_STACK_SIZE)
    # Test that the CLEANSTACK rule is consensus critical in tapscript
    add_spender(spenders, "tapscript/cleanstack", leaf="t36", tap=tap, inputs=[b'\x01'], failure={"inputs": [b'\x01', b'\x01']}, **ERR_CLEANSTACK)

    # == Test for sigops ratio limit ==

    # Given a number n, and a public key pk, functions that produce a (CScript, sigops). Each script takes as
    # input a valid signature with the passed pk followed by a dummy push of bytes that are to be dropped, and
    # will execute sigops signature checks.
    SIGOPS_RATIO_SCRIPTS = [
        # n OP_CHECKSIGVERIFYs and 1 OP_CHECKSIG.
        lambda n, pk: (CScript([OP_DROP, pk] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_CHECKSIG]), n + 1),
        # n OP_CHECKSIGVERIFYs and 1 OP_CHECKSIGADD, but also one unexecuted OP_CHECKSIGVERIFY.
        lambda n, pk: (CScript([OP_DROP, pk, OP_0, OP_IF, OP_2DUP, OP_CHECKSIGVERIFY, OP_ENDIF] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_2, OP_SWAP, OP_CHECKSIGADD, OP_3, OP_EQUAL]), n + 1),
        # n OP_CHECKSIGVERIFYs and 1 OP_CHECKSIGADD, but also one unexecuted OP_CHECKSIG.
        lambda n, pk: (CScript([random.randbytes(220), OP_2DROP, pk, OP_1, OP_NOTIF, OP_2DUP, OP_CHECKSIG, OP_VERIFY, OP_ENDIF] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_4, OP_SWAP, OP_CHECKSIGADD, OP_5, OP_EQUAL]), n + 1),
        # n OP_CHECKSIGVERIFYs and 1 OP_CHECKSIGADD, but also one unexecuted OP_CHECKSIGADD.
        lambda n, pk: (CScript([OP_DROP, pk, OP_1, OP_IF, OP_ELSE, OP_2DUP, OP_6, OP_SWAP, OP_CHECKSIGADD, OP_7, OP_EQUALVERIFY, OP_ENDIF] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_8, OP_SWAP, OP_CHECKSIGADD, OP_9, OP_EQUAL]), n + 1),
        # n+1 OP_CHECKSIGs, but also one OP_CHECKSIG with an empty signature.
        lambda n, pk: (CScript([OP_DROP, OP_0, pk, OP_CHECKSIG, OP_NOT, OP_VERIFY, pk] + [OP_2DUP, OP_CHECKSIG, OP_VERIFY] * n + [OP_CHECKSIG]), n + 1),
        # n OP_CHECKSIGADDs and 1 OP_CHECKSIG, but also an OP_CHECKSIGADD with an empty signature.
        lambda n, pk: (CScript([OP_DROP, OP_0, OP_10, pk, OP_CHECKSIGADD, OP_10, OP_EQUALVERIFY, pk] + [OP_2DUP, OP_16, OP_SWAP, OP_CHECKSIGADD, b'\x11', OP_EQUALVERIFY] * n + [OP_CHECKSIG]), n + 1),
    ]
    for annex in [None, bytes([ANNEX_TAG]) + random.randbytes(random.randrange(1000))]:
        for hashtype in [SIGHASH_DEFAULT, SIGHASH_ALL]:
            for pubkey in [pubs[1], random.randbytes(random.choice([x for x in range(2, 81) if x != 32]))]:
                for fn_num, fn in enumerate(SIGOPS_RATIO_SCRIPTS):
                    merkledepth = random.randrange(129)


                    def predict_sigops_ratio(n, dummy_size):
                        """Predict whether spending fn(n, pubkey) with dummy_size will pass the ratio test."""
                        script, sigops = fn(n, pubkey)
                        # Predict the size of the witness for a given choice of n
                        stacklen_size = 1
                        sig_size = 64 + (hashtype != SIGHASH_DEFAULT)
                        siglen_size = 1
                        dummylen_size = 1 + 2 * (dummy_size >= 253)
                        script_size = len(script)
                        scriptlen_size = 1 + 2 * (script_size >= 253)
                        control_size = 33 + 32 * merkledepth
                        controllen_size = 1 + 2 * (control_size >= 253)
                        annex_size = 0 if annex is None else len(annex)
                        annexlen_size = 0 if annex is None else 1 + 2 * (annex_size >= 253)
                        witsize = stacklen_size + sig_size + siglen_size + dummy_size + dummylen_size + script_size + scriptlen_size + control_size + controllen_size + annex_size + annexlen_size
                        # sigops ratio test
                        return witsize + 50 >= 50 * sigops
                    # Make sure n is high enough that with empty dummy, the script is not valid
                    n = 0
                    while predict_sigops_ratio(n, 0):
                        n += 1
                    # But allow picking a bit higher still
                    n += random.randrange(5)
                    # Now pick dummy size *just* large enough that the overall construction passes
                    dummylen = 0
                    while not predict_sigops_ratio(n, dummylen):
                        dummylen += 1
                    scripts = [("s", fn(n, pubkey)[0])]
                    for _ in range(merkledepth):
                        scripts = [scripts, random.choice(PARTNER_MERKLE_FN)]
                    tap = taproot_construct(pubs[0], scripts)
                    standard = annex is None and dummylen <= 80 and len(pubkey) == 32
                    add_spender(spenders, "tapscript/sigopsratio_%i" % fn_num, tap=tap, leaf="s", annex=annex, hashtype=hashtype, key=secs[1], inputs=[getter("sign"), random.randbytes(dummylen)], standard=standard, failure={"inputs": [getter("sign"), random.randbytes(dummylen - 1)]}, **ERR_TAPSCRIPT_VALIDATION_WEIGHT)

    # Future leaf versions
    for leafver in range(0, 0x100, 2):
        if leafver == LEAF_VERSION_TAPSCRIPT or leafver == ANNEX_TAG:
            # Skip the defined LEAF_VERSION_TAPSCRIPT, and the ANNEX_TAG which is not usable as leaf version
            continue
        scripts = [
            ("bare_c0", CScript([OP_NOP])),
            ("bare_unkver", CScript([OP_NOP]), leafver),
            ("return_c0", CScript([OP_RETURN])),
            ("return_unkver", CScript([OP_RETURN]), leafver),
            ("undecodable_c0", CScript([OP_PUSHDATA1])),
            ("undecodable_unkver", CScript([OP_PUSHDATA1]), leafver),
            ("bigpush_c0", CScript([random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP])),
            ("bigpush_unkver", CScript([random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP]), leafver),
            ("1001push_c0", CScript([OP_0] * 1001)),
            ("1001push_unkver", CScript([OP_0] * 1001), leafver),
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "unkver/bare", standard=False, tap=tap, leaf="bare_unkver", failure={"leaf": "bare_c0"}, **ERR_CLEANSTACK)
        add_spender(spenders, "unkver/return", standard=False, tap=tap, leaf="return_unkver", failure={"leaf": "return_c0"}, **ERR_OP_RETURN)
        add_spender(spenders, "unkver/undecodable", standard=False, tap=tap, leaf="undecodable_unkver", failure={"leaf": "undecodable_c0"}, **ERR_BAD_OPCODE)
        add_spender(spenders, "unkver/bigpush", standard=False, tap=tap, leaf="bigpush_unkver", failure={"leaf": "bigpush_c0"}, **ERR_PUSH_SIZE)
        add_spender(spenders, "unkver/1001push", standard=False, tap=tap, leaf="1001push_unkver", failure={"leaf": "1001push_c0"}, **ERR_STACK_SIZE)
        add_spender(spenders, "unkver/1001inputs", standard=False, tap=tap, leaf="bare_unkver", inputs=[b'']*1001, failure={"leaf": "bare_c0"}, **ERR_STACK_SIZE)

    # OP_SUCCESSx tests.
    def hashtype(_):
        return random.choice(VALID_SIGHASHES_TAPROOT)

    for opval in range(76, 0x100):
        opcode = CScriptOp(opval)
        if not is_op_success(opcode):
            continue
        scripts = [
            ("bare_success", CScript([opcode])),
            ("bare_nop", CScript([OP_NOP])),
            ("unexecif_success", CScript([OP_0, OP_IF, opcode, OP_ENDIF])),
            ("unexecif_nop", CScript([OP_0, OP_IF, OP_NOP, OP_ENDIF])),
            ("return_success", CScript([OP_RETURN, opcode])),
            ("return_nop", CScript([OP_RETURN, OP_NOP])),
            ("undecodable_success", CScript([opcode, OP_PUSHDATA1])),
            ("undecodable_nop", CScript([OP_NOP, OP_PUSHDATA1])),
            ("undecodable_bypassed_success", CScript([OP_PUSHDATA1, OP_2, opcode])),
            ("bigpush_success", CScript([random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP, opcode])),
            ("bigpush_nop", CScript([random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP, OP_NOP])),
            ("1001push_success", CScript([OP_0] * 1001 + [opcode])),
            ("1001push_nop", CScript([OP_0] * 1001 + [OP_NOP])),
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "opsuccess/bare", standard=False, tap=tap, leaf="bare_success", failure={"leaf": "bare_nop"}, **ERR_CLEANSTACK)
        add_spender(spenders, "opsuccess/unexecif", standard=False, tap=tap, leaf="unexecif_success", failure={"leaf": "unexecif_nop"}, **ERR_CLEANSTACK)
        add_spender(spenders, "opsuccess/return", standard=False, tap=tap, leaf="return_success", failure={"leaf": "return_nop"}, **ERR_OP_RETURN)
        add_spender(spenders, "opsuccess/undecodable", standard=False, tap=tap, leaf="undecodable_success", failure={"leaf": "undecodable_nop"}, **ERR_BAD_OPCODE)
        add_spender(spenders, "opsuccess/undecodable_bypass", standard=False, tap=tap, leaf="undecodable_success", failure={"leaf": "undecodable_bypassed_success"}, **ERR_BAD_OPCODE)
        add_spender(spenders, "opsuccess/bigpush", standard=False, tap=tap, leaf="bigpush_success", failure={"leaf": "bigpush_nop"}, **ERR_PUSH_SIZE)
        add_spender(spenders, "opsuccess/1001push", standard=False, tap=tap, leaf="1001push_success", failure={"leaf": "1001push_nop"}, **ERR_STACK_SIZE)
        add_spender(spenders, "opsuccess/1001inputs", standard=False, tap=tap, leaf="bare_success", inputs=[b'']*1001, failure={"leaf": "bare_nop"}, **ERR_STACK_SIZE)
        add_spender(spenders, "opsuccess/bigstackelem", standard=False, tap=tap, leaf="bare_success", inputs=[random.randbytes(MAX_SCRIPT_ELEMENT_SIZE+1)], failure={"leaf": "bare_nop"}, **ERR_PUSH_SIZE)

    # Non-OP_SUCCESSx (verify that those aren't accidentally treated as OP_SUCCESSx)
    for opval in range(0, 0x100):
        opcode = CScriptOp(opval)
        if is_op_success(opcode):
            continue
        scripts = [
            ("normal", CScript([OP_RETURN, opcode] + [OP_NOP] * 75)),
            ("op_success", CScript([OP_RETURN, CScriptOp(0x50)]))
        ]
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "alwaysvalid/notsuccessx", tap=tap, leaf="op_success", inputs=[], standard=False, failure={"leaf": "normal"}) # err_msg differs based on opcode

    # == Test case for https://github.com/bitcoin/bitcoin/issues/24765 ==

    def zero_fn(h):
        return bytes([0] * 32)

    tap = taproot_construct(pubs[0], [("leaf", CScript([pubs[1], OP_CHECKSIG, pubs[1], OP_CHECKSIGADD, OP_2, OP_EQUAL])), zero_fn])
    add_spender(spenders, "case24765", tap=tap, leaf="leaf", inputs=[getter("sign"), getter("sign")], key=secs[1], no_fail=True)

    # == Legacy tests ==

    # Also add a few legacy spends into the mix, so that transactions which combine taproot and pre-taproot spends get tested too.
    for compressed in [False, True]:
        eckey1, pubkey1 = generate_keypair(compressed=compressed)
        eckey2, _ = generate_keypair(compressed=compressed)
        for p2sh in [False, True]:
            for witv0 in [False, True]:
                for hashtype in VALID_SIGHASHES_ECDSA + [random.randrange(0x04, 0x80), random.randrange(0x84, 0x100)]:
                    standard = (hashtype in VALID_SIGHASHES_ECDSA) and (compressed or not witv0)
                    add_spender(spenders, "legacy/pk-wrongkey", hashtype=hashtype, p2sh=p2sh, witv0=witv0, standard=standard, script=key_to_p2pk_script(pubkey1), **SINGLE_SIG, key=eckey1, failure={"key": eckey2}, sigops_weight=4-3*witv0, **ERR_EVAL_FALSE)
                    add_spender(spenders, "legacy/pkh-sighashflip", hashtype=hashtype, p2sh=p2sh, witv0=witv0, standard=standard, pkh=pubkey1, key=eckey1, **SIGHASH_BITFLIP, sigops_weight=4-3*witv0, **ERR_EVAL_FALSE)

    # Verify that OP_CHECKSIGADD wasn't accidentally added to pre-taproot validation logic.
    for p2sh in [False, True]:
        for witv0 in [False, True]:
            for hashtype in VALID_SIGHASHES_ECDSA + [random.randrange(0x04, 0x80), random.randrange(0x84, 0x100)]:
                standard = hashtype in VALID_SIGHASHES_ECDSA and (p2sh or witv0)
                add_spender(spenders, "compat/nocsa", hashtype=hashtype, p2sh=p2sh, witv0=witv0, standard=standard, script=CScript([OP_IF, OP_11, pubkey1, OP_CHECKSIGADD, OP_12, OP_EQUAL, OP_ELSE, pubkey1, OP_CHECKSIG, OP_ENDIF]), key=eckey1, sigops_weight=4-3*witv0, inputs=[getter("sign"), b''], failure={"inputs": [getter("sign"), b'\x01']}, **ERR_BAD_OPCODE)

    # == sighash caching tests ==

    # Sighash caching in legacy.
    for p2sh in [False, True]:
        for witv0 in [False, True]:
            eckey1, pubkey1 = generate_keypair(compressed=compressed)
            for _ in range(10):
                # Construct a script with 20 checksig operations (10 sighash types, each 2 times),
                # randomly ordered and interleaved with 4 OP_CODESEPARATORS.
                ops = [1, 2, 3, 0x21, 0x42, 0x63, 0x81, 0x83, 0xe1, 0xc2, -1, -1] * 2
                # Make sure no OP_CODESEPARATOR appears last.
                while True:
                    random.shuffle(ops)
                    if ops[-1] != -1:
                        break
                script = [pubkey1]
                inputs = []
                codeseps = -1
                for pos, op in enumerate(ops):
                    if op == -1:
                        codeseps += 1
                        script.append(OP_CODESEPARATOR)
                    elif pos + 1 != len(ops):
                        script += [OP_TUCK, OP_CHECKSIGVERIFY]
                        inputs.append(getter("sign", codesepnum=codeseps, hashtype=op))
                    else:
                        script += [OP_CHECKSIG]
                        inputs.append(getter("sign", codesepnum=codeseps, hashtype=op))
                inputs.reverse()
                script = CScript(script)
                add_spender(spenders, "sighashcache/legacy", p2sh=p2sh, witv0=witv0, standard=False, script=script, inputs=inputs, key=eckey1, sigops_weight=12*8*(4-3*witv0), no_fail=True)

    # Sighash caching in tapscript.
    for _ in range(10):
        # Construct a script with 700 checksig operations (7 sighash types, each 100 times),
        # randomly ordered and interleaved with 100 OP_CODESEPARATORS.
        ops = [0, 1, 2, 3, 0x81, 0x82, 0x83, -1] * 100
        # Make sure no OP_CODESEPARATOR appears last.
        while True:
            random.shuffle(ops)
            if ops[-1] != -1:
                 break
        script = [pubs[1]]
        inputs = []
        opcount = 1
        codeseppos = 0xffffffff
        for pos, op in enumerate(ops):
            if op == -1:
                codeseppos = opcount
                opcount += 1
                script.append(OP_CODESEPARATOR)
            elif pos + 1 != len(ops):
                opcount += 2
                script += [OP_TUCK, OP_CHECKSIGVERIFY]
                inputs.append(getter("sign", codeseppos=codeseppos, hashtype=op))
            else:
                opcount += 1
                script += [OP_CHECKSIG]
                inputs.append(getter("sign", codeseppos=codeseppos, hashtype=op))
        inputs.reverse()
        script = CScript(script)
        tap = taproot_construct(pubs[0], [("leaf", script)])
        add_spender(spenders, "sighashcache/taproot", tap=tap, leaf="leaf", inputs=inputs, standard=True, key=secs[1], no_fail=True)

    return spenders


def spenders_taproot_nonstandard():
    """Spenders for testing that post-activation Taproot rules may be nonstandard."""

    spenders = []

    sec = generate_privkey()
    pub, _ = compute_xonly_pubkey(sec)
    scripts = [
        ("future_leaf", CScript([pub, OP_CHECKSIG]), 0xc2),
        ("op_success", CScript([pub, OP_CHECKSIG, OP_0, OP_IF, CScriptOp(0x50), OP_ENDIF])),
    ]
    tap = taproot_construct(pub, scripts)

    # Test that features like annex, leaf versions, or OP_SUCCESS are valid but non-standard
    add_spender(spenders, "inactive/scriptpath_valid_unkleaf", key=sec, tap=tap, leaf="future_leaf", standard=False, inputs=[getter("sign")])
    add_spender(spenders, "inactive/scriptpath_invalid_unkleaf", key=sec, tap=tap, leaf="future_leaf", standard=False, inputs=[getter("sign")], sighash=bitflipper(default_sighash))
    add_spender(spenders, "inactive/scriptpath_valid_opsuccess", key=sec, tap=tap, leaf="op_success", standard=False, inputs=[getter("sign")])
    add_spender(spenders, "inactive/scriptpath_valid_opsuccess", key=sec, tap=tap, leaf="op_success", standard=False, inputs=[getter("sign")], sighash=bitflipper(default_sighash))

    return spenders

def sample_spenders():

    # Create key(s) for output creation, as well as key and script-spends
    secs = [generate_privkey() for _ in range(2)]
    pubs = [compute_xonly_pubkey(sec)[0] for sec in secs]

    # Create a list of scripts which will be built into a taptree
    scripts = [
        # leaf label, followed by CScript
        ("2byte_push", CScript([OP_DROP, b'\xaa\xaa'])),
        ("nonstd_2byte_push", CScript.fromhex("4c02aaaa")),
        ("dummyleaf", CScript([])),
    ]

    # Build TaprootInfo using scripts and appropriate pubkey for output creation
    tap = taproot_construct(pubs[0], scripts)

    # Finally, add spender(s).
    # Each spender embodies a test with an optional failure condition.
    # These failure conditions allow for fine-grained success/failure
    # conditions that are tested randomly.
    spenders = []

    # Named comment, using first leaf from scripts, with empty string as witness data, no optional fail condition
    add_spender(spenders, comment="tutorial/push", tap=tap, leaf="2byte_push", inputs=[b'\x00'], no_fail=True)

    # Spender with alternative failure tapscript via over-riding "failure" dictionary, along with the failure's expected err_msg / ERR_*
    add_spender(spenders, comment="tutorial/pushredux", tap=tap, leaf="2byte_push", inputs=[b'\x00'], failure={"leaf": "dummyleaf"}, **ERR_EVAL_FALSE)

    # Spender that is non-standard but otherwise valid, with extraneous signature data from inner key for optional failure condition
    add_spender(spenders, comment="tutorial/nonminpush", tap=tap, leaf="nonstd_2byte_push", key=secs[0], standard=False, failure={"inputs": [getter("sign")]}, **ERR_CLEANSTACK)

    # New scripts=[] can be defined, and rinse-repeated as necessary until the spenders list is returned for execution
    return spenders


class TaprootTest(SpenderTestMixin, BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def gen_test_vectors(self):
        """Run a scenario that corresponds (and optionally produces) to BIP341 test vectors."""

        self.log.info("Unit test scenario...")

        # Deterministically mine coins to OP_TRUE in block 1
        assert_equal(self.nodes[0].getblockcount(), 0)
        coinbase = CTransaction()
        coinbase.version = 1
        coinbase.vin = [CTxIn(COutPoint(0, 0xffffffff), CScript([OP_1, OP_1]), SEQUENCE_FINAL)]
        coinbase.vout = [CTxOut(5000000000, CScript([OP_1]))]
        coinbase.nLockTime = 0
        assert_equal(coinbase.txid_hex, "f60c73405d499a956d3162e3483c395526ef78286458a4cb17b125aa92e49b20")
        # Mine it
        block = create_block(hashprev=int(self.nodes[0].getbestblockhash(), 16), coinbase=coinbase)
        block.solve()
        self.nodes[0].submitblock(block.serialize().hex())
        assert_equal(self.nodes[0].getblockcount(), 1)
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY, self.nodesigner.getnewaddress()[2])

        SEED = 317
        VALID_LEAF_VERS = list(range(0xc0, 0x100, 2)) + [0x66, 0x7e, 0x80, 0x84, 0x96, 0x98, 0xba, 0xbc, 0xbe]
        # Generate private keys
        prvs = [hashlib.sha256(SEED.to_bytes(2, 'big') + bytes([i])).digest() for i in range(100)]
        # Generate corresponding public x-only pubkeys
        pubs = [compute_xonly_pubkey(prv)[0] for prv in prvs]
        # Generate taproot objects
        inner_keys = [pubs[i] for i in range(7)]

        script_lists = [
            None,
            [("0", CScript([pubs[50], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT)],
            [("0", CScript([pubs[51], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT)],
            [("0", CScript([pubs[52], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT), ("1", CScript([b"BIP341"]), VALID_LEAF_VERS[pubs[99][0] % 41])],
            [("0", CScript([pubs[53], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT), ("1", CScript([b"Taproot"]), VALID_LEAF_VERS[pubs[99][1] % 41])],
            [("0", CScript([pubs[54], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT),
                [("1", CScript([pubs[55], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT), ("2", CScript([pubs[56], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT)]
            ],
            [("0", CScript([pubs[57], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT),
                [("1", CScript([pubs[58], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT), ("2", CScript([pubs[59], OP_CHECKSIG]), LEAF_VERSION_TAPSCRIPT)]
            ],
        ]
        taps = [taproot_construct(inner_keys[i], script_lists[i]) for i in range(len(inner_keys))]

        # Require negated taps[0]
        assert taps[0].negflag
        # Require one negated and one non-negated in taps 1 and 2.
        assert_not_equal(taps[1].negflag, taps[2].negflag)
        # Require one negated and one non-negated in taps 3 and 4.
        assert_not_equal(taps[3].negflag, taps[4].negflag)
        # Require one negated and one non-negated in taps 5 and 6.
        assert_not_equal(taps[5].negflag, taps[6].negflag)

        cblks = [{leaf: get({**DEFAULT_CONTEXT, 'tap': taps[i], 'leaf': leaf}, 'controlblock') for leaf in taps[i].leaves} for i in range(7)]
        # Require one swapped and one unswapped in taps 3 and 4.
        assert_not_equal((cblks[3]['0'][33:65] < cblks[3]['1'][33:65]), (cblks[4]['0'][33:65] < cblks[4]['1'][33:65]))
        # Require one swapped and one unswapped in taps 5 and 6, both at the top and child level.
        assert_not_equal((cblks[5]['0'][33:65] < cblks[5]['1'][65:]), (cblks[6]['0'][33:65] < cblks[6]['1'][65:]))
        assert_not_equal((cblks[5]['1'][33:65] < cblks[5]['2'][33:65]), (cblks[6]['1'][33:65] < cblks[6]['2'][33:65]))
        # Require within taps 5 (and thus also 6) that one level is swapped and the other is not.
        assert_not_equal((cblks[5]['0'][33:65] < cblks[5]['1'][65:]), (cblks[5]['1'][33:65] < cblks[5]['2'][33:65]))

        # Compute a deterministic set of scriptPubKeys
        tap_spks = []
        old_spks = []
        spend_info = {}
        # First, taproot scriptPubKeys, for the tap objects constructed above
        for i, tap in enumerate(taps):
            tap_spks.append(tap.scriptPubKey)
            d = {'key': prvs[i], 'tap': tap, 'mode': 'taproot'}
            spend_info[tap.scriptPubKey] = d
        # Then, a number of deterministically generated (keys 0x1,0x2,0x3) with 2x P2PKH, 1x P2WPKH spks.
        for i in range(1, 4):
            prv = ECKey()
            prv.set(i.to_bytes(32, 'big'), True)
            pub = prv.get_pubkey().get_bytes()
            d = {"key": prv}
            d["scriptcode"] = key_to_p2pkh_script(pub)
            d["inputs"] = [getter("sign"), pub]
            if i < 3:
                # P2PKH
                d['spk'] = key_to_p2pkh_script(pub)
                d['mode'] = 'legacy'
            else:
                # P2WPKH
                d['spk'] = key_to_p2wpkh_script(pub)
                d['mode'] = 'witv0'
            old_spks.append(d['spk'])
            spend_info[d['spk']] = d

        # Construct a deterministic chain of transactions creating UTXOs to the test's spk's (so that they
        # come from distinct txids).
        txn = []
        lasttxid = coinbase.txid_int
        amount = 5000000000
        for i, spk in enumerate(old_spks + tap_spks):
            val = 42000000 * (i + 7)
            tx = CTransaction()
            tx.version = 1
            tx.vin = [CTxIn(COutPoint(lasttxid, i & 1), CScript([]), SEQUENCE_FINAL)]
            tx.vout = [CTxOut(val, spk), CTxOut(amount - val, CScript([OP_1]))]
            if i & 1:
                tx.vout = list(reversed(tx.vout))
            tx.nLockTime = 0
            amount -= val
            lasttxid = tx.txid_int
            txn.append(tx)
            spend_info[spk]['prevout'] = COutPoint(tx.txid_int, i & 1)
            spend_info[spk]['utxo'] = CTxOut(val, spk)
        # Mine those transactions
        self.init_blockinfo(self.nodes[0])
        self.block_submit(self.nodes[0], txn, "Crediting txn", None, sigops_weight=10, accept=True)

        # scriptPubKey computation
        tests = {"version": 1}
        spk_tests = tests.setdefault("scriptPubKey", [])
        for i, tap in enumerate(taps):
            test_case = {}
            given = test_case.setdefault("given", {})
            given['internalPubkey'] = tap.internal_pubkey.hex()

            def pr(node):
                if node is None:
                    return None
                elif isinstance(node, tuple):
                    return {"id": int(node[0]), "script": node[1].hex(), "leafVersion": node[2]}
                elif len(node) == 1:
                    return pr(node[0])
                elif len(node) == 2:
                    return [pr(node[0]), pr(node[1])]
                else:
                    assert False

            given['scriptTree'] = pr(script_lists[i])
            intermediary = test_case.setdefault("intermediary", {})
            if len(tap.leaves):
                leafhashes = intermediary.setdefault('leafHashes', [None] * len(tap.leaves))
                for leaf in tap.leaves:
                    leafhashes[int(leaf)] = tap.leaves[leaf].leaf_hash.hex()
            intermediary['merkleRoot'] = tap.merkle_root.hex() if tap.merkle_root else None
            intermediary['tweak'] = tap.tweak.hex()
            intermediary['tweakedPubkey'] = tap.output_pubkey.hex()
            expected = test_case.setdefault("expected", {})
            expected['scriptPubKey'] = tap.scriptPubKey.hex()
            expected['bip350Address'] = program_to_witness(1, bytes(tap.output_pubkey), True)
            if len(tap.leaves):
                control_blocks = expected.setdefault("scriptPathControlBlocks", [None] * len(tap.leaves))
                for leaf in tap.leaves:
                    ctx = {**DEFAULT_CONTEXT, 'tap': tap, 'leaf': leaf}
                    control_blocks[int(leaf)] = get(ctx, "controlblock").hex()
            spk_tests.append(test_case)

        # Construct a deterministic transaction spending all outputs created above.
        tx = CTransaction()
        tx.version = 2
        tx.vin = []
        inputs = []
        input_spks = [tap_spks[0], tap_spks[1], old_spks[0], tap_spks[2], tap_spks[5], old_spks[2], tap_spks[6], tap_spks[3], tap_spks[4]]
        sequences = [0, SEQUENCE_FINAL, SEQUENCE_FINAL, 0xfffffffe, 0xfffffffe, 0, 0, SEQUENCE_FINAL, SEQUENCE_FINAL]
        hashtypes = [SIGHASH_SINGLE, SIGHASH_SINGLE|SIGHASH_ANYONECANPAY, SIGHASH_ALL, SIGHASH_ALL, SIGHASH_DEFAULT, SIGHASH_ALL, SIGHASH_NONE, SIGHASH_NONE|SIGHASH_ANYONECANPAY, SIGHASH_ALL|SIGHASH_ANYONECANPAY]
        for i, spk in enumerate(input_spks):
            tx.vin.append(CTxIn(spend_info[spk]['prevout'], CScript(), sequences[i]))
            inputs.append(spend_info[spk]['utxo'])
        tx.vout.append(CTxOut(1000000000, old_spks[1]))
        tx.vout.append(CTxOut(3410000000, pubs[98]))
        tx.nLockTime = 500000000
        precomputed = {
            "hashAmounts": BIP341_sha_amounts(inputs),
            "hashPrevouts": BIP341_sha_prevouts(tx),
            "hashScriptPubkeys": BIP341_sha_scriptpubkeys(inputs),
            "hashSequences": BIP341_sha_sequences(tx),
            "hashOutputs": BIP341_sha_outputs(tx)
        }
        keypath_tests = tests.setdefault("keyPathSpending", [])
        tx_test = {}
        global_given = tx_test.setdefault("given", {})
        global_given['rawUnsignedTx'] = tx.serialize().hex()
        utxos_spent = global_given.setdefault("utxosSpent", [])
        for i in range(len(input_spks)):
            utxos_spent.append({"scriptPubKey": inputs[i].scriptPubKey.hex(), "amountSats": inputs[i].nValue})
        global_intermediary = tx_test.setdefault("intermediary", {})
        for key in sorted(precomputed.keys()):
            global_intermediary[key] = precomputed[key].hex()
        test_list = tx_test.setdefault('inputSpending', [])
        for i in range(len(input_spks)):
            ctx = {
                **DEFAULT_CONTEXT,
                **spend_info[input_spks[i]],
                'tx': tx,
                'utxos': inputs,
                'idx': i,
                'hashtype': hashtypes[i],
                'deterministic': True
            }
            if ctx['mode'] == 'taproot':
                test_case = {}
                given = test_case.setdefault("given", {})
                given['txinIndex'] = i
                given['internalPrivkey'] = get(ctx, 'key').hex()
                if get(ctx, "tap").merkle_root != bytes():
                    given['merkleRoot'] = get(ctx, "tap").merkle_root.hex()
                else:
                    given['merkleRoot'] = None
                given['hashType'] = get(ctx, "hashtype")
                intermediary = test_case.setdefault("intermediary", {})
                intermediary['internalPubkey'] = get(ctx, "tap").internal_pubkey.hex()
                intermediary['tweak'] = get(ctx, "tap").tweak.hex()
                intermediary['tweakedPrivkey'] = get(ctx, "key_tweaked").hex()
                sigmsg = get(ctx, "sigmsg")
                intermediary['sigMsg'] = sigmsg.hex()
                intermediary['precomputedUsed'] = [key for key in sorted(precomputed.keys()) if sigmsg.count(precomputed[key])]
                intermediary['sigHash'] = get(ctx, "sighash").hex()
                expected = test_case.setdefault("expected", {})
                expected['witness'] = [get(ctx, "sign").hex()]
                test_list.append(test_case)
            tx.wit.vtxinwit.append(CTxInWitness())
            tx.vin[i].scriptSig = CScript(flatten(get(ctx, "scriptsig")))
            tx.wit.vtxinwit[i].scriptWitness.stack = flatten(get(ctx, "witness"))
        aux = tx_test.setdefault("auxiliary", {})
        aux['fullySignedTx'] = tx.serialize().hex()
        keypath_tests.append(tx_test)
        assert_equal(hashlib.sha256(tx.serialize()).hexdigest(), "24bab662cb55a7f3bae29b559f651674c62bcc1cd442d44715c0133939107b38")
        # Mine the spending transaction
        self.block_submit(self.nodes[0], [tx], "Spending txn", None, sigops_weight=10000, accept=True, witness=True)

        if GEN_TEST_VECTORS:
            print(json.dumps(tests, indent=4, sort_keys=False))

    def run_test(self):
        self.nodesigner = NodeSigner(self.nodes[0])
        self.gen_test_vectors()

        self.log.info("Post-activation tests...")

        # New sub-tests not checking standardness can be added to consensus_spenders
        # to allow for increased coverage across input types.
        # See sample_spenders for a minimal example
        consensus_spenders = sample_spenders()
        consensus_spenders += spenders_taproot_active()
        self.test_spenders(self.nodes[0], consensus_spenders, input_counts=[1, 2, 2, 2, 2, 3])

        # Run each test twice; once in isolation, and once combined with others. Testing in isolation
        # means that the standardness is verified in every test (as combined transactions are only standard
        # when all their inputs are standard).
        nonstd_spenders = spenders_taproot_nonstandard()
        self.test_spenders(self.nodes[0], nonstd_spenders, input_counts=[1])
        self.test_spenders(self.nodes[0], nonstd_spenders, input_counts=[2, 3])


if __name__ == '__main__':
    TaprootTest(__file__).main()
