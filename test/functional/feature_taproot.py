#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test Taproot softfork (BIPs 340-342)

from test_framework.blocktools import (
    create_coinbase,
    create_block,
    add_witness_commitment,
    MAX_BLOCK_SIGOPS_WEIGHT,
    NORMAL_GBT_REQUEST_PARAMS,
    WITNESS_SCALE_FACTOR,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    ToHex,
)
from test_framework.script import (
    ANNEX_TAG,
    CScript,
    CScriptNum,
    CScriptOp,
    LEAF_VERSION_TAPSCRIPT,
    LegacySignatureHash,
    LOCKTIME_THRESHOLD,
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
    OP_HASH160,
    OP_IF,
    OP_NOP,
    OP_NOT,
    OP_NOTIF,
    OP_PUSHDATA1,
    OP_RETURN,
    OP_SWAP,
    OP_VERIFY,
    SIGHASH_DEFAULT,
    SIGHASH_ALL,
    SIGHASH_NONE,
    SIGHASH_SINGLE,
    SIGHASH_ANYONECANPAY,
    SegwitV0SignatureHash,
    TaprootSignatureHash,
    is_op_success,
    taproot_construct,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error, assert_equal
from test_framework.key import generate_privkey, compute_xonly_pubkey, sign_schnorr, tweak_add_privkey, ECKey
from test_framework.address import (
    hash160,
    sha256,
)
from collections import OrderedDict, namedtuple
from io import BytesIO
import json
import hashlib
import os
import random

# === Framework for building spending transactions. ===
#
# The computation is represented as a "context" dict, whose entries store potentially-unevaluated expressions that
# refer to lower-level ones. By overwriting these expression, many aspects - both high and low level - of the signing
# process can be overridden.
#
# Specifically, a context object is a dict that maps names to compositions of:
# - values
# - lists of values
# - callables which, when fed the context object as argument, produce any of these
#
# The DEFAULT_CONTEXT object specifies a standard signing process, with many overridable knobs.
#
# The get(ctx, name) function can evaluate a name, and cache its result in the context.
# getter(name) can be used to construct a callable that evaluates name. For example:
#
#   ctx1 = {**DEFAULT_CONTEXT, inputs=[getter("sign"), b'\x01']}
#
# creates a context where the script inputs are a signature plus the bytes 0x01.
#
# override(expr, name1=expr1, name2=expr2, ...) can be used to cause an expression to be evaluated in a selectively
# modified context. For example:
#
#   ctx2 = {**DEFAULT_CONTEXT, sighash=override(default_sighash, hashtype=SIGHASH_DEFAULT)}
#
# creates a context ctx2 where the sighash is modified to use hashtype=SIGHASH_DEFAULT. This differs from
#
#   ctx3 = {**DEFAULT_CONTEXT, hashtype=SIGHASH_DEFAULT}
#
# in that ctx3 will globally use hashtype=SIGHASH_DEFAULT (including in the hashtype byte appended to the signature)
# while ctx2 only uses the modified hashtype inside the sighash calculation.

def deep_eval(ctx, expr):
    """Recursively replace any callables c in expr (including inside lists) with c(ctx)."""
    while callable(expr):
        expr = expr(ctx)
    if isinstance(expr, list):
        expr = [deep_eval(ctx, x) for x in expr]
    return expr

# Data type to represent fully-evaluated expressions in a context dict (so we can avoid reevaluating them).
Final = namedtuple("Final", "value")

def get(ctx, name):
    """Evaluate name in context ctx."""
    assert name in ctx, "Missing '%s' in context" % name
    expr = ctx[name]
    if not isinstance(expr, Final):
        # Evaluate and cache the result.
        expr = Final(deep_eval(ctx, expr))
        ctx[name] = expr
    return expr.value

def getter(name):
    """Return a callable that evaluates name in its passed context."""
    return lambda ctx: get(ctx, name)

def override(expr, **kwargs):
    """Return a callable that evaluates expr in a modified context."""
    return lambda ctx: deep_eval({**ctx, **kwargs}, expr)

# === Implementations for the various default expressions in DEFAULT_CONTEXT ===

def default_hashtype(ctx):
    """Default expression for "hashtype": SIGHASH_DEFAULT for taproot, SIGHASH_ALL otherwise."""
    mode = get(ctx, "mode")
    if mode == "taproot":
        return SIGHASH_DEFAULT
    else:
        return SIGHASH_ALL

def default_tapleaf(ctx):
    """Default expression for "tapleaf": looking up leaf in tap[2]."""
    return get(ctx, "tap").leaves[get(ctx, "leaf")]

def default_script_taproot(ctx):
    """Default expression for "script_taproot": tapleaf.script."""
    return get(ctx, "tapleaf").script

def default_leafversion(ctx):
    """Default expression for "leafversion": tapleaf.version"""
    return get(ctx, "tapleaf").version

def default_negflag(ctx):
    """Default expression for "negflag": tap.negflag."""
    return get(ctx, "tap").negflag

def default_pubkey_internal(ctx):
    """Default expression for "pubkey_internal": tap.internal_pubkey."""
    return get(ctx, "tap").internal_pubkey

def default_merklebranch(ctx):
    """Default expression for "merklebranch": tapleaf.merklebranch."""
    return get(ctx, "tapleaf").merklebranch

def default_controlblock(ctx):
    """Default expression for "controlblock": combine leafversion, negflag, pubkey_internal, merklebranch."""
    return bytes([get(ctx, "leafversion") + get(ctx, "negflag")]) + get(ctx, "pubkey_internal") + get(ctx, "merklebranch")

def default_sighash(ctx):
    """Default expression for "sighash": depending on mode, compute BIP341, BIP143, or legacy sighash."""
    tx = get(ctx, "tx")
    idx = get(ctx, "idx")
    hashtype = get(ctx, "hashtype_actual")
    mode = get(ctx, "mode")
    if mode == "taproot":
        # BIP341 signature hash
        utxos = get(ctx, "utxos")
        annex = get(ctx, "annex")
        if get(ctx, "leaf") is not None:
            codeseppos = get(ctx, "codeseppos")
            leaf_ver = get(ctx, "leafversion")
            script = get(ctx, "script_taproot")
            return TaprootSignatureHash(tx, utxos, hashtype, idx, scriptpath=True, script=script, leaf_ver=leaf_ver, codeseparator_pos=codeseppos, annex=annex)
        else:
            return TaprootSignatureHash(tx, utxos, hashtype, idx, scriptpath=False, annex=annex)
    elif mode == "witv0":
        # BIP143 signature hash
        scriptcode = get(ctx, "scriptcode")
        utxos = get(ctx, "utxos")
        return SegwitV0SignatureHash(scriptcode, tx, idx, hashtype, utxos[idx].nValue)
    else:
        # Pre-segwit signature hash
        scriptcode = get(ctx, "scriptcode")
        return LegacySignatureHash(scriptcode, tx, idx, hashtype)[0]

def default_tweak(ctx):
    """Default expression for "tweak": None if a leaf is specified, tap[0] otherwise."""
    if get(ctx, "leaf") is None:
        return get(ctx, "tap").tweak
    return None

def default_key_tweaked(ctx):
    """Default expression for "key_tweaked": key if tweak is None, tweaked with it otherwise."""
    key = get(ctx, "key")
    tweak = get(ctx, "tweak")
    if tweak is None:
        return key
    else:
        return tweak_add_privkey(key, tweak)

def default_signature(ctx):
    """Default expression for "signature": BIP340 signature or ECDSA signature depending on mode."""
    sighash = get(ctx, "sighash")
    if get(ctx, "mode") == "taproot":
        key = get(ctx, "key_tweaked")
        flip_r = get(ctx, "flag_flip_r")
        flip_p = get(ctx, "flag_flip_p")
        return sign_schnorr(key, sighash, flip_r=flip_r, flip_p=flip_p)
    else:
        key = get(ctx, "key")
        return key.sign_ecdsa(sighash)

def default_hashtype_actual(ctx):
    """Default expression for "hashtype_actual": hashtype, unless mismatching SIGHASH_SINGLE in taproot."""
    hashtype = get(ctx, "hashtype")
    mode = get(ctx, "mode")
    if mode != "taproot":
        return hashtype
    idx = get(ctx, "idx")
    tx = get(ctx, "tx")
    if hashtype & 3 == SIGHASH_SINGLE and idx >= len(tx.vout):
        return (hashtype & ~3) | SIGHASH_NONE
    return hashtype

def default_bytes_hashtype(ctx):
    """Default expression for "bytes_hashtype": bytes([hashtype_actual]) if not 0, b"" otherwise."""
    return bytes([x for x in [get(ctx, "hashtype_actual")] if x != 0])

def default_sign(ctx):
    """Default expression for "sign": concatenation of signature and bytes_hashtype."""
    return get(ctx, "signature") + get(ctx, "bytes_hashtype")

def default_inputs_keypath(ctx):
    """Default expression for "inputs_keypath": a signature."""
    return [get(ctx, "sign")]

def default_witness_taproot(ctx):
    """Default expression for "witness_taproot", consisting of inputs, script, control block, and annex as needed."""
    annex = get(ctx, "annex")
    suffix_annex = []
    if annex is not None:
        suffix_annex = [annex]
    if get(ctx, "leaf") is None:
        return get(ctx, "inputs_keypath") + suffix_annex
    else:
        return get(ctx, "inputs") + [bytes(get(ctx, "script_taproot")), get(ctx, "controlblock")] + suffix_annex

def default_witness_witv0(ctx):
    """Default expression for "witness_witv0", consisting of inputs and witness script, as needed."""
    script = get(ctx, "script_witv0")
    inputs = get(ctx, "inputs")
    if script is None:
        return inputs
    else:
        return inputs + [script]

def default_witness(ctx):
    """Default expression for "witness", delegating to "witness_taproot" or "witness_witv0" as needed."""
    mode = get(ctx, "mode")
    if mode == "taproot":
        return get(ctx, "witness_taproot")
    elif mode == "witv0":
        return get(ctx, "witness_witv0")
    else:
        return []

def default_scriptsig(ctx):
    """Default expression for "scriptsig", consisting of inputs and redeemscript, as needed."""
    scriptsig = []
    mode = get(ctx, "mode")
    if mode == "legacy":
        scriptsig = get(ctx, "inputs")
    redeemscript = get(ctx, "script_p2sh")
    if redeemscript is not None:
        scriptsig += [bytes(redeemscript)]
    return scriptsig

# The default context object.
DEFAULT_CONTEXT = {
    # == The main expressions to evaluate. Only override these for unusual or invalid spends. ==
    # The overall witness stack, as a list of bytes objects.
    "witness": default_witness,
    # The overall scriptsig, as a list of CScript objects (to be concatenated) and bytes objects (to be pushed)
    "scriptsig": default_scriptsig,

    # == Expressions you'll generally only override for intentionally invalid spends. ==
    # The witness stack for spending a taproot output.
    "witness_taproot": default_witness_taproot,
    # The witness stack for spending a P2WPKH/P2WSH output.
    "witness_witv0": default_witness_witv0,
    # The script inputs for a taproot key path spend.
    "inputs_keypath": default_inputs_keypath,
    # The actual hashtype to use (usually equal to hashtype, but in taproot SIGHASH_SINGLE is not always allowed).
    "hashtype_actual": default_hashtype_actual,
    # The bytes object for a full signature (including hashtype byte, if needed).
    "bytes_hashtype": default_bytes_hashtype,
    # A full script signature (bytes including hashtype, if needed)
    "sign": default_sign,
    # An ECDSA or Schnorr signature (excluding hashtype byte).
    "signature": default_signature,
    # The 32-byte tweaked key (equal to key for script path spends, or key+tweak for key path spends).
    "key_tweaked": default_key_tweaked,
    # The tweak to use (None for script path spends, the actual tweak for key path spends).
    "tweak": default_tweak,
    # The sighash value (32 bytes)
    "sighash": default_sighash,
    # The information about the chosen script path spend (TaprootLeafInfo object).
    "tapleaf": default_tapleaf,
    # The script to push, and include in the sighash, for a taproot script path spend.
    "script_taproot": default_script_taproot,
    # The internal pubkey for a taproot script path spend (32 bytes).
    "pubkey_internal": default_pubkey_internal,
    # The negation flag of the internal pubkey for a taproot script path spend.
    "negflag": default_negflag,
    # The leaf version to include in the sighash (this does not affect the one in the control block).
    "leafversion": default_leafversion,
    # The Merkle path to include in the control block for a script path spend.
    "merklebranch": default_merklebranch,
    # The control block to push for a taproot script path spend.
    "controlblock": default_controlblock,
    # Whether to produce signatures with invalid P sign (Schnorr signatures only).
    "flag_flip_p": False,
    # Whether to produce signatures with invalid R sign (Schnorr signatures only).
    "flag_flip_r": False,

    # == Parameters that can be changed without invalidating, but do have a default: ==
    # The hashtype (as an integer).
    "hashtype": default_hashtype,
    # The annex (only when mode=="taproot").
    "annex": None,
    # The codeseparator position (only when mode=="taproot").
    "codeseppos": -1,
    # The redeemscript to add to the scriptSig (if P2SH; None implies not P2SH).
    "script_p2sh": None,
    # The script to add to the witness in (if P2WSH; None implies P2WPKH)
    "script_witv0": None,
    # The leaf to use in taproot spends (if script path spend; None implies key path spend).
    "leaf": None,
    # The input arguments to provide to the executed script
    "inputs": [],

    # == Parameters to be set before evaluation: ==
    # - mode: what spending style to use ("taproot", "witv0", or "legacy").
    # - key: the (untweaked) private key to sign with (ECKey object for ECDSA, 32 bytes for Schnorr).
    # - tap: the TaprootInfo object (see taproot_construct; needed in mode=="taproot").
    # - tx: the transaction to sign.
    # - utxos: the UTXOs being spent (needed in mode=="witv0" and mode=="taproot").
    # - idx: the input position being signed.
    # - scriptcode: the scriptcode to include in legacy and witv0 sighashes.
}

def flatten(lst):
    ret = []
    for elem in lst:
        if isinstance(elem, list):
            ret += flatten(elem)
        else:
            ret.append(elem)
    return ret

def spend(tx, idx, utxos, **kwargs):
    """Sign transaction input idx of tx, provided utxos is the list of outputs being spent.

    Additional arguments may be provided that override any aspect of the signing process.
    See DEFAULT_CONTEXT above for what can be overridden, and what must be provided.
    """

    ctx = {**DEFAULT_CONTEXT, "tx":tx, "idx":idx, "utxos":utxos, **kwargs}

    def to_script(elem):
        """If fed a CScript, return it; if fed bytes, return a CScript that pushes it."""
        if isinstance(elem, CScript):
            return elem
        else:
            return CScript([elem])

    scriptsig_list = flatten(get(ctx, "scriptsig"))
    scriptsig = CScript(b"".join(bytes(to_script(elem)) for elem in scriptsig_list))
    witness_stack = flatten(get(ctx, "witness"))
    return (scriptsig, witness_stack)


# === Spender objects ===
#
# Each spender is a tuple of:
# - A scriptPubKey which is to be spent from (CScript)
# - A comment describing the test (string)
# - Whether the spending (on itself) is expected to be standard (bool)
# - A tx-signing lambda returning (scriptsig, witness_stack), taking as inputs:
#   - A transaction to sign (CTransaction)
#   - An input position (int)
#   - The spent UTXOs by this transaction (list of CTxOut)
#   - Whether to produce a valid spend (bool)
# - A string with an expected error message for failure case if known
# - The (pre-taproot) sigops weight consumed by a successful spend
# - Whether this spend cannot fail
# - Whether this test demands being placed in a txin with no corresponding txout (for testing SIGHASH_SINGLE behavior)

Spender = namedtuple("Spender", "script,comment,is_standard,sat_function,err_msg,sigops_weight,no_fail,need_vin_vout_mismatch")

def make_spender(comment, *, tap=None, witv0=False, script=None, pkh=None, p2sh=False, spk_mutate_pre_p2sh=None, failure=None, standard=True, err_msg=None, sigops_weight=0, need_vin_vout_mismatch=False, **kwargs):
    """Helper for constructing Spender objects using the context signing framework.

    * tap: a TaprootInfo object (see taproot_construct), for Taproot spends (cannot be combined with pkh, witv0, or script)
    * witv0: boolean indicating the use of witness v0 spending (needs one of script or pkh)
    * script: the actual script executed (for bare/P2WSH/P2SH spending)
    * pkh: the public key for P2PKH or P2WPKH spending
    * p2sh: whether the output is P2SH wrapper (this is supported even for Taproot, where it makes the output unencumbered)
    * spk_mutate_pre_psh: a callable to be applied to the script (before potentially P2SH-wrapping it)
    * failure: a dict of entries to override in the context when intentionally failing to spend (if None, no_fail will be set)
    * standard: whether the (valid version of) spending is expected to be standard
    * err_msg: a string with an expected error message for failure (or None, if not cared about)
    * sigops_weight: the pre-taproot sigops weight consumed by a successful spend
    * need_vin_vout_mismatch: whether this test requires being tested in a transaction input that has no corresponding
                              transaction output.
    """

    conf = dict()

    # Compute scriptPubKey and set useful defaults based on the inputs.
    if witv0:
        assert tap is None
        conf["mode"] = "witv0"
        if pkh is not None:
            # P2WPKH
            assert script is None
            pubkeyhash = hash160(pkh)
            spk = CScript([OP_0, pubkeyhash])
            conf["scriptcode"] = CScript([OP_DUP, OP_HASH160, pubkeyhash, OP_EQUALVERIFY, OP_CHECKSIG])
            conf["script_witv0"] = None
            conf["inputs"] = [getter("sign"), pkh]
        elif script is not None:
            # P2WSH
            spk = CScript([OP_0, sha256(script)])
            conf["scriptcode"] = script
            conf["script_witv0"] = script
        else:
            assert False
    elif tap is None:
        conf["mode"] = "legacy"
        if pkh is not None:
            # P2PKH
            assert script is None
            pubkeyhash = hash160(pkh)
            spk = CScript([OP_DUP, OP_HASH160, pubkeyhash, OP_EQUALVERIFY, OP_CHECKSIG])
            conf["scriptcode"] = spk
            conf["inputs"] = [getter("sign"), pkh]
        elif script is not None:
            # bare
            spk = script
            conf["scriptcode"] = script
        else:
            assert False
    else:
        assert script is None
        conf["mode"] = "taproot"
        conf["tap"] = tap
        spk = tap.scriptPubKey

    if spk_mutate_pre_p2sh is not None:
        spk = spk_mutate_pre_p2sh(spk)

    if p2sh:
        # P2SH wrapper can be combined with anything else
        conf["script_p2sh"] = spk
        spk = CScript([OP_HASH160, hash160(spk), OP_EQUAL])

    conf = {**conf, **kwargs}

    def sat_fn(tx, idx, utxos, valid):
        if valid:
            return spend(tx, idx, utxos, **conf)
        else:
            assert failure is not None
            return spend(tx, idx, utxos, **{**conf, **failure})

    return Spender(script=spk, comment=comment, is_standard=standard, sat_function=sat_fn, err_msg=err_msg, sigops_weight=sigops_weight, no_fail=failure is None, need_vin_vout_mismatch=need_vin_vout_mismatch)

def add_spender(spenders, *args, **kwargs):
    """Make a spender using make_spender, and add it to spenders."""
    spenders.append(make_spender(*args, **kwargs))

# === Helpers for the test ===

def random_checksig_style(pubkey):
    """Creates a random CHECKSIG* tapscript that would succeed with only the valid signature on witness stack."""
    opcode = random.choice([OP_CHECKSIG, OP_CHECKSIGVERIFY, OP_CHECKSIGADD])
    if (opcode == OP_CHECKSIGVERIFY):
        ret = CScript([pubkey, opcode, OP_1])
    elif (opcode == OP_CHECKSIGADD):
        num = random.choice([0, 0x7fffffff, -0x7fffffff])
        ret = CScript([num, pubkey, opcode, num + 1, OP_EQUAL])
    else:
        ret = CScript([pubkey, opcode])
    return bytes(ret)

def random_bytes(n):
    """Return a random bytes object of length n."""
    return bytes(random.getrandbits(8) for i in range(n))

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

# Expected error strings

ERR_SIG_SIZE = {"err_msg": "Invalid Schnorr signature size"}
ERR_SIG_HASHTYPE = {"err_msg": "Invalid Schnorr signature hash type"}
ERR_SIG_SCHNORR = {"err_msg": "Invalid Schnorr signature"}
ERR_OP_RETURN = {"err_msg": "OP_RETURN was encountered"}
ERR_CONTROLBLOCK_SIZE = {"err_msg": "Invalid Taproot control block size"}
ERR_WITNESS_PROGRAM_MISMATCH = {"err_msg": "Witness program hash mismatch"}
ERR_PUSH_LIMIT = {"err_msg": "Push value size limit exceeded"}
ERR_DISABLED_OPCODE = {"err_msg": "Attempted to use a disabled opcode"}
ERR_TAPSCRIPT_CHECKMULTISIG = {"err_msg": "OP_CHECKMULTISIG(VERIFY) is not available in tapscript"}
ERR_MINIMALIF = {"err_msg": "OP_IF/NOTIF argument must be minimal in tapscript"}
ERR_UNKNOWN_PUBKEY = {"err_msg": "Public key is neither compressed or uncompressed"}
ERR_STACK_SIZE = {"err_msg": "Stack size limit exceeded"}
ERR_CLEANSTACK = {"err_msg": "Stack size must be exactly one after execution"}
ERR_STACK_EMPTY = {"err_msg": "Operation not valid with the current stack size"}
ERR_SIGOPS_RATIO = {"err_msg": "Too much signature validation relative to witness weight"}
ERR_UNDECODABLE = {"err_msg": "Opcode missing or not understood"}
ERR_NO_SUCCESS = {"err_msg": "Script evaluated without error but finished with a false/empty top stack element"}
ERR_EMPTY_WITNESS = {"err_msg": "Witness program was passed an empty witness"}
ERR_CHECKSIGVERIFY = {"err_msg": "Script failed an OP_CHECKSIGVERIFY operation"}

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

DUST_LIMIT = 600
MIN_FEE = 50000

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
    add_spender(spenders, "sig/key", tap=tap, key=secs[0], failure={"key_tweaked": bitflipper(default_key_tweaked)}, **ERR_SIG_SCHNORR)
    # Test with sighash with bit flipped.
    add_spender(spenders, "sig/sighash", tap=tap, key=secs[0], failure={"sighash": bitflipper(default_sighash)}, **ERR_SIG_SCHNORR)
    # Test with invalid R sign.
    add_spender(spenders, "sig/flip_r", tap=tap, key=secs[0], failure={"flag_flip_r": True}, **ERR_SIG_SCHNORR)
    # Test with invalid P sign.
    add_spender(spenders, "sig/flip_p", tap=tap, key=secs[0], failure={"flag_flip_p": True}, **ERR_SIG_SCHNORR)
    # Test with signature with bit flipped.
    add_spender(spenders, "sig/bitflip", tap=tap, key=secs[0], failure={"signature": bitflipper(default_signature)}, **ERR_SIG_SCHNORR)

    # == Tests for signature hashing ==

    # Run all tests once with no annex, and once with a valid random annex.
    for annex in [None, lambda _: bytes([ANNEX_TAG]) + random_bytes(random.randrange(0, 250))]:
        # Non-empty annex is non-standard
        no_annex = annex is None

        # Sighash mutation tests (test all sighash combinations)
        for hashtype in VALID_SIGHASHES_TAPROOT:
            common = {"annex": annex, "hashtype": hashtype, "standard": no_annex}

            # Pure pubkey
            tap = taproot_construct(pubs[0])
            add_spender(spenders, "sighash/purepk", tap=tap, key=secs[0], **common, **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)

            # Pubkey/P2PK script combination
            scripts = [("s0", CScript(random_checksig_style(pubs[1])))]
            tap = taproot_construct(pubs[0], scripts)
            add_spender(spenders, "sighash/keypath_hashtype_%x" % hashtype, tap=tap, key=secs[0], **common, **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)
            add_spender(spenders, "sighash/scriptpath_hashtype_%x" % hashtype, tap=tap, leaf="s0", key=secs[1], **common, **SINGLE_SIG, **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)

            # Test SIGHASH_SINGLE behavior in combination with mismatching outputs
            if hashtype in VALID_SIGHASHES_TAPROOT_SINGLE:
                add_spender(spenders, "sighash/keypath_hashtype_mis_%x" % hashtype, tap=tap, key=secs[0], annex=annex, standard=no_annex, hashtype_actual=random.choice(VALID_SIGHASHES_TAPROOT_NO_SINGLE), failure={"hashtype_actual": hashtype}, **ERR_SIG_HASHTYPE, need_vin_vout_mismatch=True)
                add_spender(spenders, "sighash/scriptpath_hashtype_mis_%x" % hashtype, tap=tap, leaf="s0", key=secs[1], annex=annex, standard=no_annex, hashtype_actual=random.choice(VALID_SIGHASHES_TAPROOT_NO_SINGLE), **SINGLE_SIG, failure={"hashtype_actual": hashtype}, **ERR_SIG_HASHTYPE, need_vin_vout_mismatch=True)

        # Test OP_CODESEPARATOR impact on sighashing.
        hashtype = lambda _: random.choice(VALID_SIGHASHES_TAPROOT)
        common = {"annex": annex, "hashtype": hashtype, "standard": no_annex}
        scripts = [
            ("pk_codesep", CScript(random_checksig_style(pubs[1]) + bytes([OP_CODESEPARATOR]))),  # codesep after checksig
            ("codesep_pk", CScript(bytes([OP_CODESEPARATOR]) + random_checksig_style(pubs[1]))),  # codesep before checksig
            ("branched_codesep", CScript([random_bytes(random.randrange(511)), OP_DROP, OP_IF, OP_CODESEPARATOR, pubs[0], OP_ELSE, OP_CODESEPARATOR, pubs[1], OP_ENDIF, OP_CHECKSIG])),  # branch dependent codesep
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "sighash/pk_codesep", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)
        add_spender(spenders, "sighash/codesep_pk", tap=tap, leaf="codesep_pk", key=secs[1], codeseppos=0, **common, **SINGLE_SIG, **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)
        add_spender(spenders, "sighash/branched_codesep/left", tap=tap, leaf="branched_codesep", key=secs[0], codeseppos=3, **common, inputs=[getter("sign"), b'\x01'], **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)
        add_spender(spenders, "sighash/branched_codesep/right", tap=tap, leaf="branched_codesep", key=secs[1], codeseppos=6, **common, inputs=[getter("sign"), b''], **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)

    # Reusing the scripts above, test that various features affect the sighash.
    add_spender(spenders, "sighash/annex", tap=tap, leaf="pk_codesep", key=secs[1], hashtype=hashtype, standard=False, **SINGLE_SIG, annex=bytes([ANNEX_TAG]), failure={"sighash": override(default_sighash, annex=None)}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/script", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, failure={"sighash": override(default_sighash, script_taproot=tap.leaves["codesep_pk"].script)}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/leafver", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, failure={"sighash": override(default_sighash, leafversion=random.choice([x & 0xFE for x in range(0x100) if x & 0xFE != 0xC0]))}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **common, **SINGLE_SIG, failure={"sighash": override(default_sighash, leaf=None)}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/keypath", tap=tap, key=secs[0], **common, failure={"sighash": override(default_sighash, leaf="pk_codesep")}, **ERR_SIG_SCHNORR)

    # Test that invalid hashtypes don't work, both in key path and script path spends
    hashtype = lambda _: random.choice(VALID_SIGHASHES_TAPROOT)
    for invalid_hashtype in [x for x in range(0x100) if x not in VALID_SIGHASHES_TAPROOT]:
        add_spender(spenders, "sighash/keypath_unk_hashtype_%x" % invalid_hashtype, tap=tap, key=secs[0], hashtype=hashtype, failure={"hashtype": invalid_hashtype}, **ERR_SIG_HASHTYPE)
        add_spender(spenders, "sighash/scriptpath_unk_hashtype_%x" % invalid_hashtype, tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=hashtype, failure={"hashtype": invalid_hashtype}, **ERR_SIG_HASHTYPE)

    # Test that hashtype 0 cannot have a hashtype byte, and 1 must have one.
    add_spender(spenders, "sighash/hashtype0_byte_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_DEFAULT])}, **ERR_SIG_HASHTYPE)
    add_spender(spenders, "sighash/hashtype0_byte_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_DEFAULT])}, **ERR_SIG_HASHTYPE)
    add_spender(spenders, "sighash/hashtype1_byte_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/hashtype1_byte_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SIG_SCHNORR)
    # Test that hashtype 0 and hashtype 1 cannot be transmuted into each other.
    add_spender(spenders, "sighash/hashtype0to1_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_ALL])}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/hashtype0to1_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_DEFAULT, failure={"bytes_hashtype": bytes([SIGHASH_ALL])}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/hashtype1to0_keypath", tap=tap, key=secs[0], hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "sighash/hashtype1to0_scriptpath", tap=tap, leaf="pk_codesep", key=secs[1], **SINGLE_SIG, hashtype=SIGHASH_ALL, failure={"bytes_hashtype": b''}, **ERR_SIG_SCHNORR)

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
        add_spender(spenders, "siglen/empty_keypath", tap=tap, key=secs[3], hashtype=hashtype, failure={"sign": b""}, **ERR_SIG_SIZE)
        add_spender(spenders, "siglen/empty_csv", tap=tap, key=secs[2], leaf="csv", hashtype=hashtype, **SINGLE_SIG, failure={"sign": b""}, **ERR_CHECKSIGVERIFY)
        add_spender(spenders, "siglen/empty_cs", tap=tap, key=secs[2], leaf="cs_pos", hashtype=hashtype, **SINGLE_SIG, failure={"sign": b""}, **ERR_NO_SUCCESS)
        add_spender(spenders, "siglen/empty_csa", tap=tap, key=secs[2], leaf="csa_pos", hashtype=hashtype, **SINGLE_SIG, failure={"sign": b""}, **ERR_NO_SUCCESS)
        add_spender(spenders, "siglen/empty_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": lambda _: random_bytes(random.randrange(1, 63))}, **ERR_SIG_SIZE)
        add_spender(spenders, "siglen/empty_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": lambda _: random_bytes(random.randrange(66, 100))}, **ERR_SIG_SIZE)
        # Appending a zero byte to signatures invalidates them
        add_spender(spenders, "siglen/padzero_keypath", tap=tap, key=secs[3], hashtype=hashtype, **SIG_ADD_ZERO, **(ERR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_csv", tap=tap, key=secs[2], leaf="csv", hashtype=hashtype, **SINGLE_SIG, **SIG_ADD_ZERO, **(ERR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_cs", tap=tap, key=secs[2], leaf="cs_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_ADD_ZERO, **(ERR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_csa", tap=tap, key=secs[2], leaf="csa_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_ADD_ZERO, **(ERR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_ADD_ZERO, **(ERR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SIZE))
        add_spender(spenders, "siglen/padzero_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_ADD_ZERO, **(ERR_SIG_HASHTYPE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SIZE))
        # Removing the last byte from signatures invalidates them
        add_spender(spenders, "siglen/popbyte_keypath", tap=tap, key=secs[3], hashtype=hashtype, **SIG_POP_BYTE, **(ERR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SCHNORR))
        add_spender(spenders, "siglen/popbyte_csv", tap=tap, key=secs[2], leaf="csv", hashtype=hashtype, **SINGLE_SIG, **SIG_POP_BYTE, **(ERR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SCHNORR))
        add_spender(spenders, "siglen/popbyte_cs", tap=tap, key=secs[2], leaf="cs_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_POP_BYTE, **(ERR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SCHNORR))
        add_spender(spenders, "siglen/popbyte_csa", tap=tap, key=secs[2], leaf="csa_pos", hashtype=hashtype, **SINGLE_SIG, **SIG_POP_BYTE, **(ERR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SCHNORR))
        add_spender(spenders, "siglen/popbyte_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_POP_BYTE, **(ERR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SCHNORR))
        add_spender(spenders, "siglen/popbyte_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", **SIG_POP_BYTE, **(ERR_SIG_SIZE if hashtype == SIGHASH_DEFAULT else ERR_SIG_SCHNORR))
        # Verify that an invalid signature is not allowed, not even when the CHECKSIG* is expected to fail.
        add_spender(spenders, "siglen/invalid_cs_neg", tap=tap, key=secs[2], leaf="cs_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": default_sign, "sighash": bitflipper(default_sighash)}, **ERR_SIG_SCHNORR)
        add_spender(spenders, "siglen/invalid_csa_neg", tap=tap, key=secs[2], leaf="csa_neg", hashtype=hashtype, **SINGLE_SIG, sign=b"", failure={"sign": default_sign, "sighash": bitflipper(default_sighash)}, **ERR_SIG_SCHNORR)

    # == Test that BIP341 spending only applies to witness version 1, program length 32, no P2SH ==

    for p2sh in [False, True]:
        for witver in range(1, 17):
            for witlen in [20, 31, 32, 33]:
                def mutate(spk):
                    prog = spk[2:]
                    assert len(prog) == 32
                    if witlen < 32:
                        prog = prog[0:witlen]
                    elif witlen > 32:
                        prog += bytes([0 for _ in range(witlen - 32)])
                    return CScript([CScriptOp.encode_op_n(witver), prog])
                scripts = [("s0", CScript([pubs[0], OP_CHECKSIG])), ("dummy", CScript([OP_RETURN]))]
                tap = taproot_construct(pubs[1], scripts)
                if not p2sh and witver == 1 and witlen == 32:
                    add_spender(spenders, "applic/keypath", p2sh=p2sh, spk_mutate_pre_p2sh=mutate, tap=tap, key=secs[1], **SIGHASH_BITFLIP, **ERR_SIG_SCHNORR)
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
    add_spender(spenders, "spendpath/merklelimit", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"leaf": "129deep"}, **ERR_CONTROLBLOCK_SIZE)
    # Test that flipping the negation bit invalidates spends.
    add_spender(spenders, "spendpath/negflag", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"negflag": lambda ctx: 1 - default_negflag(ctx)}, **ERR_WITNESS_PROGRAM_MISMATCH)
    # Test that bitflips in the Merkle branch invalidate it.
    add_spender(spenders, "spendpath/bitflipmerkle", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"merklebranch": bitflipper(default_merklebranch)}, **ERR_WITNESS_PROGRAM_MISMATCH)
    # Test that bitflips in the internal pubkey invalidate it.
    add_spender(spenders, "spendpath/bitflippubkey", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"pubkey_internal": bitflipper(default_pubkey_internal)}, **ERR_WITNESS_PROGRAM_MISMATCH)
    # Test that empty witnesses are invalid.
    add_spender(spenders, "spendpath/emptywit", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"witness": []}, **ERR_EMPTY_WITNESS)
    # Test that adding garbage to the control block invalidates it.
    add_spender(spenders, "spendpath/padlongcontrol", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_controlblock(ctx) + random_bytes(random.randrange(1, 32))}, **ERR_CONTROLBLOCK_SIZE)
    # Test that truncating the control block invalidates it.
    add_spender(spenders, "spendpath/trunclongcontrol", tap=tap, leaf="128deep", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_merklebranch(ctx)[0:random.randrange(1, 32)]}, **ERR_CONTROLBLOCK_SIZE)

    scripts = [("s", CScript([pubs[0], OP_CHECKSIG]))]
    tap = taproot_construct(pubs[1], scripts)
    # Test that adding garbage to the control block invalidates it.
    add_spender(spenders, "spendpath/padshortcontrol", tap=tap, leaf="s", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_controlblock(ctx) + random_bytes(random.randrange(1, 32))}, **ERR_CONTROLBLOCK_SIZE)
    # Test that truncating the control block invalidates it.
    add_spender(spenders, "spendpath/truncshortcontrol", tap=tap, leaf="s", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_merklebranch(ctx)[0:random.randrange(1, 32)]}, **ERR_CONTROLBLOCK_SIZE)
    # Test that truncating the control block to 1 byte ("-1 Merkle length") invalidates it
    add_spender(spenders, "spendpath/trunc1shortcontrol", tap=tap, leaf="s", **SINGLE_SIG, key=secs[0], failure={"controlblock": lambda ctx: default_merklebranch(ctx)[0:1]}, **ERR_CONTROLBLOCK_SIZE)

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
        ("t25", CScript([random_bytes(MAX_SCRIPT_ELEMENT_SIZE), OP_DROP, pubs[1], OP_CHECKSIG])),
        # 26) Script that pushes a (MAX_SCRIPT_ELEMENT_SIZE+1)-bytes element
        ("t26", CScript([random_bytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP, pubs[1], OP_CHECKSIG])),
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
    add_spender(spenders, "tapscript/inputmaxlimit", leaf="t0", **common, standard=False, inputs=[getter("sign"), random_bytes(MAX_SCRIPT_ELEMENT_SIZE)], failure={"inputs": [getter("sign"), random_bytes(MAX_SCRIPT_ELEMENT_SIZE+1)]}, **ERR_PUSH_LIMIT)
    add_spender(spenders, "tapscript/input80limit", leaf="t0", **common, inputs=[getter("sign"), random_bytes(80)])
    add_spender(spenders, "tapscript/input81limit", leaf="t0", **common, standard=False, inputs=[getter("sign"), random_bytes(81)])
    # Test that OP_CHECKMULTISIG and OP_CHECKMULTISIGVERIFY cause failure, but OP_CHECKSIG and OP_CHECKSIGVERIFY work.
    add_spender(spenders, "tapscript/disabled_checkmultisig", leaf="t1", **common, **SINGLE_SIG, failure={"leaf": "t3"}, **ERR_TAPSCRIPT_CHECKMULTISIG)
    add_spender(spenders, "tapscript/disabled_checkmultisigverify", leaf="t2", **common, **SINGLE_SIG, failure={"leaf": "t4"}, **ERR_TAPSCRIPT_CHECKMULTISIG)
    # Test that OP_IF and OP_NOTIF do not accept non-0x01 as truth value (the MINIMALIF rule is consensus in Tapscript)
    add_spender(spenders, "tapscript/minimalif", leaf="t5", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x02']}, **ERR_MINIMALIF)
    add_spender(spenders, "tapscript/minimalnotif", leaf="t6", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x03']}, **ERR_MINIMALIF)
    add_spender(spenders, "tapscript/minimalif", leaf="t5", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x0001']}, **ERR_MINIMALIF)
    add_spender(spenders, "tapscript/minimalnotif", leaf="t6", **common, inputs=[getter("sign"), b'\x01'], failure={"inputs": [getter("sign"), b'\x0100']}, **ERR_MINIMALIF)
    # Test that 1-byte public keys (which are unknown) are acceptable but nonstandard with unrelated signatures, but 0-byte public keys are not valid.
    add_spender(spenders, "tapscript/unkpk/checksig", leaf="t16", standard=False, **common, **SINGLE_SIG, failure={"leaf": "t7"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/unkpk/checksigadd", leaf="t17", standard=False, **common, **SINGLE_SIG, failure={"leaf": "t10"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/unkpk/checksigverify", leaf="t18", standard=False, **common, **SINGLE_SIG, failure={"leaf": "t8"}, **ERR_UNKNOWN_PUBKEY)
    # Test that 33-byte public keys (which are unknown) are acceptable but nonstandard with valid signatures, but normal pubkeys are not valid in that case.
    add_spender(spenders, "tapscript/oldpk/checksig", leaf="t30", standard=False, **common, **SINGLE_SIG, sighash=bitflipper(default_sighash), failure={"leaf": "t1"}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "tapscript/oldpk/checksigadd", leaf="t31", standard=False, **common, **SINGLE_SIG, sighash=bitflipper(default_sighash), failure={"leaf": "t2"}, **ERR_SIG_SCHNORR)
    add_spender(spenders, "tapscript/oldpk/checksigverify", leaf="t32", standard=False, **common, **SINGLE_SIG, sighash=bitflipper(default_sighash), failure={"leaf": "t28"}, **ERR_SIG_SCHNORR)
    # Test that 0-byte public keys are not acceptable.
    add_spender(spenders, "tapscript/emptypk/checksig", leaf="t1", **SINGLE_SIG, **common, failure={"leaf": "t7"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/emptypk/checksigverify", leaf="t2", **SINGLE_SIG, **common, failure={"leaf": "t8"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/emptypk/checksigadd", leaf="t9", **SINGLE_SIG, **common, failure={"leaf": "t10"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/emptypk/checksigadd", leaf="t35", standard=False, **SINGLE_SIG, **common, failure={"leaf": "t10"}, **ERR_UNKNOWN_PUBKEY)
    # Test that OP_CHECKSIGADD results are as expected
    add_spender(spenders, "tapscript/checksigaddresults", leaf="t28", **SINGLE_SIG, **common, failure={"leaf": "t27"}, err_msg="unknown error")
    add_spender(spenders, "tapscript/checksigaddoversize", leaf="t29", **SINGLE_SIG, **common, failure={"leaf": "t27"}, err_msg="unknown error")
    # Test that OP_CHECKSIGADD requires 3 stack elements.
    add_spender(spenders, "tapscript/checksigadd3args", leaf="t9", **SINGLE_SIG, **common, failure={"leaf": "t11"}, **ERR_STACK_EMPTY)
    # Test that empty signatures do not cause script failure in OP_CHECKSIG and OP_CHECKSIGADD (but do fail with empty pubkey, and do fail OP_CHECKSIGVERIFY)
    add_spender(spenders, "tapscript/emptysigs/checksig", leaf="t12", **common, inputs=[b'', getter("sign")], failure={"leaf": "t13"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/emptysigs/nochecksigverify", leaf="t12", **common, inputs=[b'', getter("sign")], failure={"leaf": "t20"}, **ERR_UNKNOWN_PUBKEY)
    add_spender(spenders, "tapscript/emptysigs/checksigadd", leaf="t14", **common, inputs=[b'', getter("sign")], failure={"leaf": "t15"}, **ERR_UNKNOWN_PUBKEY)
    # Test that scripts over 10000 bytes (and over 201 non-push ops) are acceptable.
    add_spender(spenders, "tapscript/no10000limit", leaf="t19", **SINGLE_SIG, **common)
    # Test that a stack size of 1000 elements is permitted, but 1001 isn't.
    add_spender(spenders, "tapscript/1000stack", leaf="t21", **SINGLE_SIG, **common, failure={"leaf": "t22"}, **ERR_STACK_SIZE)
    # Test that an input stack size of 1000 elements is permitted, but 1001 isn't.
    add_spender(spenders, "tapscript/1000inputs", leaf="t23", **common, inputs=[getter("sign")] + [b'' for _ in range(999)], failure={"leaf": "t24", "inputs": [getter("sign")] + [b'' for _ in range(1000)]}, **ERR_STACK_SIZE)
    # Test that pushing a MAX_SCRIPT_ELEMENT_SIZE byte stack element is valid, but one longer is not.
    add_spender(spenders, "tapscript/pushmaxlimit", leaf="t25", **common, **SINGLE_SIG, failure={"leaf": "t26"}, **ERR_PUSH_LIMIT)
    # Test that 999-of-999 multisig works (but 1000-of-1000 triggers stack size limits)
    add_spender(spenders, "tapscript/bigmulti", leaf="t33", **common, inputs=big_spend_inputs, num=999, failure={"leaf": "t34", "num": 1000}, **ERR_STACK_SIZE)
    # Test that the CLEANSTACK rule is consensus critical in tapscript
    add_spender(spenders, "tapscript/cleanstack", leaf="t36", tap=tap, inputs=[b'\x01'], failure={"inputs": [b'\x01', b'\x01']}, **ERR_CLEANSTACK)

    # == Test for sigops ratio limit ==

    # Given a number n, and a public key pk, functions that produce a (CScript, sigops). Each script takes as
    # input a valid signature with the passed pk followed by a dummy push of bytes that are to be dropped, and
    # will execute sigops signature checks.
    SIGOPS_RATIO_SCRIPTS = [
        # n OP_CHECKSIGVERFIYs and 1 OP_CHECKSIG.
        lambda n, pk: (CScript([OP_DROP, pk] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_CHECKSIG]), n + 1),
        # n OP_CHECKSIGVERIFYs and 1 OP_CHECKSIGADD, but also one unexecuted OP_CHECKSIGVERIFY.
        lambda n, pk: (CScript([OP_DROP, pk, OP_0, OP_IF, OP_2DUP, OP_CHECKSIGVERIFY, OP_ENDIF] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_2, OP_SWAP, OP_CHECKSIGADD, OP_3, OP_EQUAL]), n + 1),
        # n OP_CHECKSIGVERIFYs and 1 OP_CHECKSIGADD, but also one unexecuted OP_CHECKSIG.
        lambda n, pk: (CScript([random_bytes(220), OP_2DROP, pk, OP_1, OP_NOTIF, OP_2DUP, OP_CHECKSIG, OP_VERIFY, OP_ENDIF] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_4, OP_SWAP, OP_CHECKSIGADD, OP_5, OP_EQUAL]), n + 1),
        # n OP_CHECKSIGVERFIYs and 1 OP_CHECKSIGADD, but also one unexecuted OP_CHECKSIGADD.
        lambda n, pk: (CScript([OP_DROP, pk, OP_1, OP_IF, OP_ELSE, OP_2DUP, OP_6, OP_SWAP, OP_CHECKSIGADD, OP_7, OP_EQUALVERIFY, OP_ENDIF] + [OP_2DUP, OP_CHECKSIGVERIFY] * n + [OP_8, OP_SWAP, OP_CHECKSIGADD, OP_9, OP_EQUAL]), n + 1),
        # n+1 OP_CHECKSIGs, but also one OP_CHECKSIG with an empty signature.
        lambda n, pk: (CScript([OP_DROP, OP_0, pk, OP_CHECKSIG, OP_NOT, OP_VERIFY, pk] + [OP_2DUP, OP_CHECKSIG, OP_VERIFY] * n + [OP_CHECKSIG]), n + 1),
        # n OP_CHECKSIGADDs and 1 OP_CHECKSIG, but also an OP_CHECKSIGADD with an empty signature.
        lambda n, pk: (CScript([OP_DROP, OP_0, OP_10, pk, OP_CHECKSIGADD, OP_10, OP_EQUALVERIFY, pk] + [OP_2DUP, OP_16, OP_SWAP, OP_CHECKSIGADD, b'\x11', OP_EQUALVERIFY] * n + [OP_CHECKSIG]), n + 1),
    ]
    for annex in [None, bytes([ANNEX_TAG]) + random_bytes(random.randrange(1000))]:
        for hashtype in [SIGHASH_DEFAULT, SIGHASH_ALL]:
            for pubkey in [pubs[1], random_bytes(random.choice([x for x in range(2, 81) if x != 32]))]:
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
                    add_spender(spenders, "tapscript/sigopsratio_%i" % fn_num, tap=tap, leaf="s", annex=annex, hashtype=hashtype, key=secs[1], inputs=[getter("sign"), random_bytes(dummylen)], standard=standard, failure={"inputs": [getter("sign"), random_bytes(dummylen - 1)]}, **ERR_SIGOPS_RATIO)

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
            ("bigpush_c0", CScript([random_bytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP])),
            ("bigpush_unkver", CScript([random_bytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP]), leafver),
            ("1001push_c0", CScript([OP_0] * 1001)),
            ("1001push_unkver", CScript([OP_0] * 1001), leafver),
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "unkver/bare", standard=False, tap=tap, leaf="bare_unkver", failure={"leaf": "bare_c0"}, **ERR_CLEANSTACK)
        add_spender(spenders, "unkver/return", standard=False, tap=tap, leaf="return_unkver", failure={"leaf": "return_c0"}, **ERR_OP_RETURN)
        add_spender(spenders, "unkver/undecodable", standard=False, tap=tap, leaf="undecodable_unkver", failure={"leaf": "undecodable_c0"}, **ERR_UNDECODABLE)
        add_spender(spenders, "unkver/bigpush", standard=False, tap=tap, leaf="bigpush_unkver", failure={"leaf": "bigpush_c0"}, **ERR_PUSH_LIMIT)
        add_spender(spenders, "unkver/1001push", standard=False, tap=tap, leaf="1001push_unkver", failure={"leaf": "1001push_c0"}, **ERR_STACK_SIZE)
        add_spender(spenders, "unkver/1001inputs", standard=False, tap=tap, leaf="bare_unkver", inputs=[b'']*1001, failure={"leaf": "bare_c0"}, **ERR_STACK_SIZE)

    # OP_SUCCESSx tests.
    hashtype = lambda _: random.choice(VALID_SIGHASHES_TAPROOT)
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
            ("bigpush_success", CScript([random_bytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP, opcode])),
            ("bigpush_nop", CScript([random_bytes(MAX_SCRIPT_ELEMENT_SIZE+1), OP_DROP, OP_NOP])),
            ("1001push_success", CScript([OP_0] * 1001 + [opcode])),
            ("1001push_nop", CScript([OP_0] * 1001 + [OP_NOP])),
        ]
        random.shuffle(scripts)
        tap = taproot_construct(pubs[0], scripts)
        add_spender(spenders, "opsuccess/bare", standard=False, tap=tap, leaf="bare_success", failure={"leaf": "bare_nop"}, **ERR_CLEANSTACK)
        add_spender(spenders, "opsuccess/unexecif", standard=False, tap=tap, leaf="unexecif_success", failure={"leaf": "unexecif_nop"}, **ERR_CLEANSTACK)
        add_spender(spenders, "opsuccess/return", standard=False, tap=tap, leaf="return_success", failure={"leaf": "return_nop"}, **ERR_OP_RETURN)
        add_spender(spenders, "opsuccess/undecodable", standard=False, tap=tap, leaf="undecodable_success", failure={"leaf": "undecodable_nop"}, **ERR_UNDECODABLE)
        add_spender(spenders, "opsuccess/undecodable_bypass", standard=False, tap=tap, leaf="undecodable_success", failure={"leaf": "undecodable_bypassed_success"}, **ERR_UNDECODABLE)
        add_spender(spenders, "opsuccess/bigpush", standard=False, tap=tap, leaf="bigpush_success", failure={"leaf": "bigpush_nop"}, **ERR_PUSH_LIMIT)
        add_spender(spenders, "opsuccess/1001push", standard=False, tap=tap, leaf="1001push_success", failure={"leaf": "1001push_nop"}, **ERR_STACK_SIZE)
        add_spender(spenders, "opsuccess/1001inputs", standard=False, tap=tap, leaf="bare_success", inputs=[b'']*1001, failure={"leaf": "bare_nop"}, **ERR_STACK_SIZE)

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

    # == Legacy tests ==

    # Also add a few legacy spends into the mix, so that transactions which combine taproot and pre-taproot spends get tested too.
    for compressed in [False, True]:
        eckey1 = ECKey()
        eckey1.set(generate_privkey(), compressed)
        pubkey1 = eckey1.get_pubkey().get_bytes()
        eckey2 = ECKey()
        eckey2.set(generate_privkey(), compressed)
        for p2sh in [False, True]:
            for witv0 in [False, True]:
                for hashtype in VALID_SIGHASHES_ECDSA + [random.randrange(0x04, 0x80), random.randrange(0x84, 0x100)]:
                    standard = (hashtype in VALID_SIGHASHES_ECDSA) and (compressed or not witv0)
                    add_spender(spenders, "legacy/pk-wrongkey", hashtype=hashtype, p2sh=p2sh, witv0=witv0, standard=standard, script=CScript([pubkey1, OP_CHECKSIG]), **SINGLE_SIG, key=eckey1, failure={"key": eckey2}, sigops_weight=4-3*witv0, **ERR_NO_SUCCESS)
                    add_spender(spenders, "legacy/pkh-sighashflip", hashtype=hashtype, p2sh=p2sh, witv0=witv0, standard=standard, pkh=pubkey1, key=eckey1, **SIGHASH_BITFLIP, sigops_weight=4-3*witv0, **ERR_NO_SUCCESS)

    # Verify that OP_CHECKSIGADD wasn't accidentally added to pre-taproot validation logic.
    for p2sh in [False, True]:
        for witv0 in [False, True]:
            for hashtype in VALID_SIGHASHES_ECDSA + [random.randrange(0x04, 0x80), random.randrange(0x84, 0x100)]:
                standard = hashtype in VALID_SIGHASHES_ECDSA and (p2sh or witv0)
                add_spender(spenders, "compat/nocsa", hashtype=hashtype, p2sh=p2sh, witv0=witv0, standard=standard, script=CScript([OP_IF, OP_11, pubkey1, OP_CHECKSIGADD, OP_12, OP_EQUAL, OP_ELSE, pubkey1, OP_CHECKSIG, OP_ENDIF]), key=eckey1, sigops_weight=4-3*witv0, inputs=[getter("sign"), b''], failure={"inputs": [getter("sign"), b'\x01']}, **ERR_UNDECODABLE)

    return spenders

def spenders_taproot_inactive():
    """Spenders for testing that pre-activation Taproot rules don't apply."""

    spenders = []

    sec = generate_privkey()
    pub, _ = compute_xonly_pubkey(sec)
    scripts = [
        ("pk", CScript([pub, OP_CHECKSIG])),
        ("future_leaf", CScript([pub, OP_CHECKSIG]), 0xc2),
        ("op_success", CScript([pub, OP_CHECKSIG, OP_0, OP_IF, CScriptOp(0x50), OP_ENDIF])),
    ]
    tap = taproot_construct(pub, scripts)

    # Test that keypath spending is valid & non-standard, regardless of validity.
    add_spender(spenders, "inactive/keypath_valid", key=sec, tap=tap, standard=False)
    add_spender(spenders, "inactive/keypath_invalidsig", key=sec, tap=tap, standard=False, sighash=bitflipper(default_sighash))
    add_spender(spenders, "inactive/keypath_empty", key=sec, tap=tap, standard=False, witness=[])

    # Same for scriptpath spending (and features like annex, leaf versions, or OP_SUCCESS don't change this)
    add_spender(spenders, "inactive/scriptpath_valid", key=sec, tap=tap, leaf="pk", standard=False, inputs=[getter("sign")])
    add_spender(spenders, "inactive/scriptpath_invalidsig", key=sec, tap=tap, leaf="pk", standard=False, inputs=[getter("sign")], sighash=bitflipper(default_sighash))
    add_spender(spenders, "inactive/scriptpath_invalidcb", key=sec, tap=tap, leaf="pk", standard=False, inputs=[getter("sign")], controlblock=bitflipper(default_controlblock))
    add_spender(spenders, "inactive/scriptpath_valid_unkleaf", key=sec, tap=tap, leaf="future_leaf", standard=False, inputs=[getter("sign")])
    add_spender(spenders, "inactive/scriptpath_invalid_unkleaf", key=sec, tap=tap, leaf="future_leaf", standard=False, inputs=[getter("sign")], sighash=bitflipper(default_sighash))
    add_spender(spenders, "inactive/scriptpath_valid_opsuccess", key=sec, tap=tap, leaf="op_success", standard=False, inputs=[getter("sign")])
    add_spender(spenders, "inactive/scriptpath_valid_opsuccess", key=sec, tap=tap, leaf="op_success", standard=False, inputs=[getter("sign")], sighash=bitflipper(default_sighash))

    return spenders

# Consensus validation flags to use in dumps for tests with "legacy/" or "inactive/" prefix.
LEGACY_FLAGS = "P2SH,DERSIG,CHECKLOCKTIMEVERIFY,CHECKSEQUENCEVERIFY,WITNESS,NULLDUMMY"
# Consensus validation flags to use in dumps for all other tests.
TAPROOT_FLAGS = "P2SH,DERSIG,CHECKLOCKTIMEVERIFY,CHECKSEQUENCEVERIFY,WITNESS,NULLDUMMY,TAPROOT"

def dump_json_test(tx, input_utxos, idx, success, failure):
    spender = input_utxos[idx].spender
    # Determine flags to dump
    flags = LEGACY_FLAGS if spender.comment.startswith("legacy/") or spender.comment.startswith("inactive/") else TAPROOT_FLAGS

    fields = [
        ("tx", tx.serialize().hex()),
        ("prevouts", [x.output.serialize().hex() for x in input_utxos]),
        ("index", idx),
        ("flags", flags),
        ("comment", spender.comment)
    ]

    # The "final" field indicates that a spend should be always valid, even with more validation flags enabled
    # than the listed ones. Use standardness as a proxy for this (which gives a conservative underestimate).
    if spender.is_standard:
        fields.append(("final", True))

    def dump_witness(wit):
        return OrderedDict([("scriptSig", wit[0].hex()), ("witness", [x.hex() for x in wit[1]])])
    if success is not None:
        fields.append(("success", dump_witness(success)))
    if failure is not None:
        fields.append(("failure", dump_witness(failure)))

    # Write the dump to $TEST_DUMP_DIR/x/xyz... where x,y,z,... are the SHA1 sum of the dump (which makes the
    # file naming scheme compatible with fuzzing infrastructure).
    dump = json.dumps(OrderedDict(fields)) + ",\n"
    sha1 = hashlib.sha1(dump.encode("utf-8")).hexdigest()
    dirname = os.environ.get("TEST_DUMP_DIR", ".") + ("/%s" % sha1[0])
    os.makedirs(dirname, exist_ok=True)
    with open(dirname + ("/%s" % sha1), 'w', encoding="utf8") as f:
        f.write(dump)

# Data type to keep track of UTXOs, where they were created, and how to spend them.
UTXOData = namedtuple('UTXOData', 'outpoint,output,spender')

class TaprootTest(BitcoinTestFramework):
    def add_options(self, parser):
        parser.add_argument("--dumptests", dest="dump_tests", default=False, action="store_true",
                            help="Dump generated test cases to directory set by TEST_DUMP_DIR environment variable")

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # Node 0 has Taproot inactive, Node 1 active.
        self.extra_args = [["-par=1", "-vbparams=taproot:1:1"], ["-par=1"]]

    def block_submit(self, node, txs, msg, err_msg, cb_pubkey=None, fees=0, sigops_weight=0, witness=False, accept=False):

        # Deplete block of any non-tapscript sigops using a single additional 0-value coinbase output.
        # It is not impossible to fit enough tapscript sigops to hit the old 80k limit without
        # busting txin-level limits. We simply have to account for the p2pk outputs in all
        # transactions.
        extra_output_script = CScript([OP_CHECKSIG]*((MAX_BLOCK_SIGOPS_WEIGHT - sigops_weight) // WITNESS_SCALE_FACTOR))

        block = create_block(self.tip, create_coinbase(self.lastblockheight + 1, pubkey=cb_pubkey, extra_output_script=extra_output_script, fees=fees), self.lastblocktime + 1)
        block.nVersion = 4
        for tx in txs:
            tx.rehash()
            block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        witness and add_witness_commitment(block)
        block.rehash()
        block.solve()
        block_response = node.submitblock(block.serialize().hex())
        if err_msg is not None:
            assert block_response is not None and err_msg in block_response, "Missing error message '%s' from block response '%s': %s" % (err_msg, "(None)" if block_response is None else block_response, msg)
        if (accept):
            assert node.getbestblockhash() == block.hash, "Failed to accept: %s (response: %s)" % (msg, block_response)
            self.tip = block.sha256
            self.lastblockhash = block.hash
            self.lastblocktime += 1
            self.lastblockheight += 1
        else:
            assert node.getbestblockhash() == self.lastblockhash, "Failed to reject: " + msg

    def test_spenders(self, node, spenders, input_counts):
        """Run randomized tests with a number of "spenders".

        Steps:
            1) Generate an appropriate UTXO for each spender to test spend conditions
            2) Generate 100 random addresses of all wallet types: pkh/sh_wpkh/wpkh
            3) Select random number of inputs from (1)
            4) Select random number of addresses from (2) as outputs

        Each spender embodies a test; in a large randomized test, it is verified
        that toggling the valid argument to each lambda toggles the validity of
        the transaction. This is accomplished by constructing transactions consisting
        of all valid inputs, except one invalid one.
        """

        # Construct a bunch of sPKs that send coins back to the host wallet
        self.log.info("- Constructing addresses for returning coins")
        host_spks = []
        host_pubkeys = []
        for i in range(16):
            addr = node.getnewaddress(address_type=random.choice(["legacy", "p2sh-segwit", "bech32"]))
            info = node.getaddressinfo(addr)
            spk = bytes.fromhex(info['scriptPubKey'])
            host_spks.append(spk)
            host_pubkeys.append(bytes.fromhex(info['pubkey']))

        # Initialize variables used by block_submit().
        self.lastblockhash = node.getbestblockhash()
        self.tip = int(self.lastblockhash, 16)
        block = node.getblock(self.lastblockhash)
        self.lastblockheight = block['height']
        self.lastblocktime = block['time']

        # Create transactions spending up to 50 of the wallet's inputs, with one output for each spender, and
        # one change output at the end. The transaction is constructed on the Python side to enable
        # having multiple outputs to the same address and outputs with no assigned address. The wallet
        # is then asked to sign it through signrawtransactionwithwallet, and then added to a block on the
        # Python side (to bypass standardness rules).
        self.log.info("- Creating test UTXOs...")
        random.shuffle(spenders)
        normal_utxos = []
        mismatching_utxos = [] # UTXOs with input that requires mismatching output position
        done = 0
        while done < len(spenders):
            # Compute how many UTXOs to create with this transaction
            count_this_tx = min(len(spenders) - done, (len(spenders) + 4) // 5, 10000)

            fund_tx = CTransaction()
            # Add the 50 highest-value inputs
            unspents = node.listunspent()
            random.shuffle(unspents)
            unspents.sort(key=lambda x: int(x["amount"] * 100000000), reverse=True)
            if len(unspents) > 50:
                unspents = unspents[:50]
            random.shuffle(unspents)
            balance = 0
            for unspent in unspents:
                balance += int(unspent["amount"] * 100000000)
                txid = int(unspent["txid"], 16)
                fund_tx.vin.append(CTxIn(COutPoint(txid, int(unspent["vout"])), CScript()))
            # Add outputs
            cur_progress = done / len(spenders)
            next_progress = (done + count_this_tx) / len(spenders)
            change_goal = (1.0 - 0.6 * next_progress) / (1.0 - 0.6 * cur_progress) * balance
            self.log.debug("Create %i UTXOs in a transaction spending %i inputs worth %.8f (sending ~%.8f to change)" % (count_this_tx, len(unspents), balance * 0.00000001, change_goal * 0.00000001))
            for i in range(count_this_tx):
                avg = (balance - change_goal) / (count_this_tx - i)
                amount = int(random.randrange(int(avg*0.85 + 0.5), int(avg*1.15 + 0.5)) + 0.5)
                balance -= amount
                fund_tx.vout.append(CTxOut(amount, spenders[done + i].script))
            # Add change
            fund_tx.vout.append(CTxOut(balance - 10000, random.choice(host_spks)))
            # Ask the wallet to sign
            ss = BytesIO(bytes.fromhex(node.signrawtransactionwithwallet(ToHex(fund_tx))["hex"]))
            fund_tx.deserialize(ss)
            # Construct UTXOData entries
            fund_tx.rehash()
            for i in range(count_this_tx):
                utxodata = UTXOData(outpoint=COutPoint(fund_tx.sha256, i), output=fund_tx.vout[i], spender=spenders[done])
                if utxodata.spender.need_vin_vout_mismatch:
                    mismatching_utxos.append(utxodata)
                else:
                    normal_utxos.append(utxodata)
                done += 1
            # Mine into a block
            self.block_submit(node, [fund_tx], "Funding tx", None, random.choice(host_pubkeys), 10000, MAX_BLOCK_SIGOPS_WEIGHT, True, True)

        # Consume groups of choice(input_coins) from utxos in a tx, testing the spenders.
        self.log.info("- Running %i spending tests" % done)
        random.shuffle(normal_utxos)
        random.shuffle(mismatching_utxos)
        assert done == len(normal_utxos) + len(mismatching_utxos)

        left = done
        while left:
            # Construct CTransaction with random nVersion, nLocktime
            tx = CTransaction()
            tx.nVersion = random.choice([1, 2, random.randint(-0x80000000, 0x7fffffff)])
            min_sequence = (tx.nVersion != 1 and tx.nVersion != 0) * 0x80000000  # The minimum sequence number to disable relative locktime
            if random.choice([True, False]):
                tx.nLockTime = random.randrange(LOCKTIME_THRESHOLD, self.lastblocktime - 7200)  # all absolute locktimes in the past
            else:
                tx.nLockTime = random.randrange(self.lastblockheight + 1)  # all block heights in the past

            # Decide how many UTXOs to test with.
            acceptable = [n for n in input_counts if n <= left and (left - n > max(input_counts) or (left - n) in [0] + input_counts)]
            num_inputs = random.choice(acceptable)

            # If we have UTXOs that require mismatching inputs/outputs left, include exactly one of those
            # unless there is only one normal UTXO left (as tests with mismatching UTXOs require at least one
            # normal UTXO to go in the first position), and we don't want to run out of normal UTXOs.
            input_utxos = []
            while len(mismatching_utxos) and (len(input_utxos) == 0 or len(normal_utxos) == 1):
                input_utxos.append(mismatching_utxos.pop())
                left -= 1

            # Top up until we hit num_inputs (but include at least one normal UTXO always).
            for _ in range(max(1, num_inputs - len(input_utxos))):
                input_utxos.append(normal_utxos.pop())
                left -= 1

            # The first input cannot require a mismatching output (as there is at least one output).
            while True:
                random.shuffle(input_utxos)
                if not input_utxos[0].spender.need_vin_vout_mismatch:
                    break
            first_mismatch_input = None
            for i in range(len(input_utxos)):
                if input_utxos[i].spender.need_vin_vout_mismatch:
                    first_mismatch_input = i
            assert first_mismatch_input is None or first_mismatch_input > 0

            # Decide fee, and add CTxIns to tx.
            amount = sum(utxo.output.nValue for utxo in input_utxos)
            fee = min(random.randrange(MIN_FEE * 2, MIN_FEE * 4), amount - DUST_LIMIT)  # 10000-20000 sat fee
            in_value = amount - fee
            tx.vin = [CTxIn(outpoint=utxo.outpoint, nSequence=random.randint(min_sequence, 0xffffffff)) for utxo in input_utxos]
            tx.wit.vtxinwit = [CTxInWitness() for _ in range(len(input_utxos))]
            sigops_weight = sum(utxo.spender.sigops_weight for utxo in input_utxos)
            self.log.debug("Test: %s" % (", ".join(utxo.spender.comment for utxo in input_utxos)))

            # Add 1 to 4 random outputs (but constrained by inputs that require mismatching outputs)
            num_outputs = random.choice(range(1, 1 + min(4, 4 if first_mismatch_input is None else first_mismatch_input)))
            assert in_value >= 0 and fee - num_outputs * DUST_LIMIT >= MIN_FEE
            for i in range(num_outputs):
                tx.vout.append(CTxOut())
                if in_value <= DUST_LIMIT:
                    tx.vout[-1].nValue = DUST_LIMIT
                elif i < num_outputs - 1:
                    tx.vout[-1].nValue = in_value
                else:
                    tx.vout[-1].nValue = random.randint(DUST_LIMIT, in_value)
                in_value -= tx.vout[-1].nValue
                tx.vout[-1].scriptPubKey = random.choice(host_spks)
                sigops_weight += CScript(tx.vout[-1].scriptPubKey).GetSigOpCount(False) * WITNESS_SCALE_FACTOR
            fee += in_value
            assert fee >= 0

            # Select coinbase pubkey
            cb_pubkey = random.choice(host_pubkeys)
            sigops_weight += 1 * WITNESS_SCALE_FACTOR

            # Precompute one satisfying and one failing scriptSig/witness for each input.
            input_data = []
            for i in range(len(input_utxos)):
                fn = input_utxos[i].spender.sat_function
                fail = None
                success = fn(tx, i, [utxo.output for utxo in input_utxos], True)
                if not input_utxos[i].spender.no_fail:
                    fail = fn(tx, i, [utxo.output for utxo in input_utxos], False)
                input_data.append((fail, success))
                if self.options.dump_tests:
                    dump_json_test(tx, input_utxos, i, success, fail)

            # Sign each input incorrectly once on each complete signing pass, except the very last.
            for fail_input in list(range(len(input_utxos))) + [None]:
                # Skip trying to fail at spending something that can't be made to fail.
                if fail_input is not None and input_utxos[fail_input].spender.no_fail:
                    continue
                # Expected message with each input failure, may be None(which is ignored)
                expected_fail_msg = None if fail_input is None else input_utxos[fail_input].spender.err_msg
                # Fill inputs/witnesses
                for i in range(len(input_utxos)):
                    tx.vin[i].scriptSig = input_data[i][i != fail_input][0]
                    tx.wit.vtxinwit[i].scriptWitness.stack = input_data[i][i != fail_input][1]
                # Submit to mempool to check standardness
                is_standard_tx = fail_input is None and all(utxo.spender.is_standard for utxo in input_utxos) and tx.nVersion >= 1 and tx.nVersion <= 2
                tx.rehash()
                msg = ','.join(utxo.spender.comment + ("*" if n == fail_input else "") for n, utxo in enumerate(input_utxos))
                if is_standard_tx:
                    node.sendrawtransaction(tx.serialize().hex(), 0)
                    assert node.getmempoolentry(tx.hash) is not None, "Failed to accept into mempool: " + msg
                else:
                    assert_raises_rpc_error(-26, None, node.sendrawtransaction, tx.serialize().hex(), 0)
                # Submit in a block
                self.block_submit(node, [tx], msg, witness=True, accept=fail_input is None, cb_pubkey=cb_pubkey, fees=fee, sigops_weight=sigops_weight, err_msg=expected_fail_msg)

            if (len(spenders) - left) // 200 > (len(spenders) - left - len(input_utxos)) // 200:
                self.log.info("  - %i tests done" % (len(spenders) - left))

        assert left == 0
        assert len(normal_utxos) == 0
        assert len(mismatching_utxos) == 0
        self.log.info("  - Done")

    def run_test(self):
        # Post-taproot activation tests go first (pre-taproot tests' blocks are invalid post-taproot).
        self.log.info("Post-activation tests...")
        self.nodes[1].generate(101)
        self.test_spenders(self.nodes[1], spenders_taproot_active(), input_counts=[1, 2, 2, 2, 2, 3])

        # Re-connect nodes in case they have been disconnected
        self.disconnect_nodes(0, 1)
        self.connect_nodes(0, 1)

        # Transfer value of the largest 500 coins to pre-taproot node.
        addr = self.nodes[0].getnewaddress()

        unsp = self.nodes[1].listunspent()
        unsp = sorted(unsp, key=lambda i: i['amount'], reverse=True)
        unsp = unsp[:500]

        rawtx = self.nodes[1].createrawtransaction(
            inputs=[{
                'txid': i['txid'],
                'vout': i['vout']
            } for i in unsp],
            outputs={addr: sum(i['amount'] for i in unsp)}
        )
        rawtx = self.nodes[1].signrawtransactionwithwallet(rawtx)['hex']

        # Mine a block with the transaction
        block = create_block(tmpl=self.nodes[1].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS), txlist=[rawtx])
        add_witness_commitment(block)
        block.rehash()
        block.solve()
        assert_equal(None, self.nodes[1].submitblock(block.serialize().hex()))
        self.sync_blocks()

        # Pre-taproot activation tests.
        self.log.info("Pre-activation tests...")
        # Run each test twice; once in isolation, and once combined with others. Testing in isolation
        # means that the standardness is verified in every test (as combined transactions are only standard
        # when all their inputs are standard).
        self.test_spenders(self.nodes[0], spenders_taproot_inactive(), input_counts=[1])
        self.test_spenders(self.nodes[0], spenders_taproot_inactive(), input_counts=[2, 3])


if __name__ == '__main__':
    TaprootTest().main()
