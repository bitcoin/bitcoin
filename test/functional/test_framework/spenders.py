#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Framework for building script spends and testing them against a node."""

from .blocktools import (
    create_coinbase,
    create_block,
    add_witness_commitment,
    MAX_BLOCK_SIGOPS_WEIGHT,
)
from .mempool_util import (
    TRUC_MAX_VSIZE,
)
from .messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    tx_from_hex,
    TX_MAX_STANDARD_VERSION,
    WITNESS_SCALE_FACTOR,
)
from .script import (
    CScript,
    hash256,
    LegacySignatureMsg,
    LOCKTIME_THRESHOLD,
    OP_CHECKSIG,
    OP_CODESEPARATOR,
    SIGHASH_DEFAULT,
    SIGHASH_ALL,
    SIGHASH_NONE,
    SIGHASH_SINGLE,
    SegwitV0SignatureMsg,
    TaggedHash,
    TaprootSignatureMsg,
)
from .script_util import (
    key_to_p2wpkh_script,
    keyhash_to_p2pkh_script,
    script_to_p2sh_script,
    script_to_p2wsh_script,
)
from .util import (
    assert_raises_rpc_error,
    assert_equal,
)
from .key import (
    sign_schnorr,
    tweak_add_privkey,
)
from .address import (
    hash160,
)
from collections import OrderedDict, namedtuple
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

def getter(name, **kwargs):
    """Return a callable that evaluates name in its passed context."""
    return lambda ctx: get({**ctx, **kwargs}, name)

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

def default_scriptcode_suffix(ctx):
    """Default expression for "scriptcode_suffix", the actually used portion of the scriptcode."""
    scriptcode = get(ctx, "scriptcode")
    codesepnum = get(ctx, "codesepnum")
    if codesepnum == -1:
        return scriptcode
    codeseps = 0
    for (opcode, data, sop_idx) in scriptcode.raw_iter():
        if opcode == OP_CODESEPARATOR:
            if codeseps == codesepnum:
                return CScript(scriptcode[sop_idx+1:])
            codeseps += 1
    assert False

def default_sigmsg(ctx):
    """Default expression for "sigmsg": depending on mode, compute BIP341, BIP143, or legacy sigmsg."""
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
            return TaprootSignatureMsg(tx, utxos, hashtype, idx, scriptpath=True, leaf_script=script, leaf_ver=leaf_ver, codeseparator_pos=codeseppos, annex=annex)
        else:
            return TaprootSignatureMsg(tx, utxos, hashtype, idx, scriptpath=False, annex=annex)
    elif mode == "witv0":
        # BIP143 signature hash
        scriptcode = get(ctx, "scriptcode_suffix")
        utxos = get(ctx, "utxos")
        return SegwitV0SignatureMsg(scriptcode, tx, idx, hashtype, utxos[idx].nValue)
    else:
        # Pre-segwit signature hash
        scriptcode = get(ctx, "scriptcode_suffix")
        return LegacySignatureMsg(scriptcode, tx, idx, hashtype)[0]

def default_sighash(ctx):
    """Default expression for "sighash": depending on mode, compute tagged hash or dsha256 of sigmsg."""
    msg = get(ctx, "sigmsg")
    mode = get(ctx, "mode")
    if mode == "taproot":
        return TaggedHash("TapSighash", msg)
    else:
        if msg is None:
            return (1).to_bytes(32, 'little')
        else:
            return hash256(msg)

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
    deterministic = get(ctx, "deterministic")
    if get(ctx, "mode") == "taproot":
        key = get(ctx, "key_tweaked")
        flip_r = get(ctx, "flag_flip_r")
        flip_p = get(ctx, "flag_flip_p")
        aux = bytes([0] * 32)
        if not deterministic:
            aux = random.getrandbits(256).to_bytes(32, 'big')
        return sign_schnorr(key, sighash, flip_r=flip_r, flip_p=flip_p, aux=aux)
    else:
        key = get(ctx, "key")
        return key.sign_ecdsa(sighash, rfc6979=deterministic)

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
    mode = get(ctx, "mode")
    hashtype_actual = get(ctx, "hashtype_actual")
    if mode != "taproot" or hashtype_actual != 0:
        return bytes([hashtype_actual])
    else:
        return bytes()

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
    # The part of the scriptcode after the last executed OP_CODESEPARATOR.
    "scriptcode_suffix": default_scriptcode_suffix,
    # The sigmsg value (preimage of sighash)
    "sigmsg": default_sigmsg,
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
    "codeseppos": 0xffffffff,
    # Which OP_CODESEPARATOR is the last executed one in the script (in legacy/P2SH/P2WSH).
    "codesepnum": -1,
    # The redeemscript to add to the scriptSig (if P2SH; None implies not P2SH).
    "script_p2sh": None,
    # The script to add to the witness in (if P2WSH; None implies P2WPKH)
    "script_witv0": None,
    # The leaf to use in taproot spends (if script path spend; None implies key path spend).
    "leaf": None,
    # The input arguments to provide to the executed script
    "inputs": [],
    # Use deterministic signing nonces
    "deterministic": False,

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
            spk = key_to_p2wpkh_script(pkh)
            conf["scriptcode"] = keyhash_to_p2pkh_script(pubkeyhash)
            conf["script_witv0"] = None
            conf["inputs"] = [getter("sign"), pkh]
        elif script is not None:
            # P2WSH
            spk = script_to_p2wsh_script(script)
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
            spk = keyhash_to_p2pkh_script(pubkeyhash)
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
        spk = script_to_p2sh_script(spk)

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

# Expected error strings

ERR_SCHNORR_SIG_SIZE = {"err_msg": "Invalid Schnorr signature size"}
ERR_SCHNORR_SIG_HASHTYPE = {"err_msg": "Invalid Schnorr signature hash type"}
ERR_SCHNORR_SIG = {"err_msg": "Invalid Schnorr signature"}
ERR_OP_RETURN = {"err_msg": "OP_RETURN was encountered"}
ERR_TAPROOT_WRONG_CONTROL_SIZE = {"err_msg": "Invalid Taproot control block size"}
ERR_WITNESS_PROGRAM_MISMATCH = {"err_msg": "Witness program hash mismatch"}
ERR_PUSH_SIZE = {"err_msg": "Push value size limit exceeded"}
ERR_DISABLED_OPCODE = {"err_msg": "Attempted to use a disabled opcode"}
ERR_TAPSCRIPT_CHECKMULTISIG = {"err_msg": "OP_CHECKMULTISIG(VERIFY) is not available in tapscript"}
ERR_TAPSCRIPT_MINIMALIF = {"err_msg": "OP_IF/NOTIF argument must be minimal in tapscript"}
ERR_TAPSCRIPT_EMPTY_PUBKEY = {"err_msg": "Empty public key in tapscript"}
ERR_STACK_SIZE = {"err_msg": "Stack size limit exceeded"}
ERR_CLEANSTACK = {"err_msg": "Stack size must be exactly one after execution"}
ERR_INVALID_STACK_OPERATION = {"err_msg": "Operation not valid with the current stack size"}
ERR_TAPSCRIPT_VALIDATION_WEIGHT = {"err_msg": "Too much signature validation relative to witness weight"}
ERR_BAD_OPCODE = {"err_msg": "Opcode missing or not understood"}
ERR_EVAL_FALSE = {"err_msg": "Script evaluated without error but finished with a false/empty top stack element"}
ERR_WITNESS_PROGRAM_WITNESS_EMPTY = {"err_msg": "Witness program was passed an empty witness"}
ERR_CHECKSIGVERIFY = {"err_msg": "Script failed an OP_CHECKSIGVERIFY operation"}
ERR_SCRIPT_NUM = {"err_msg": "Script number overflowed or is non-minimally encoded"}
ERR_NEGATIVE_LOCKTIME = {"err_msg": "Negative locktime"}
ERR_EQUALVERIFY = {"err_msg": "Script failed an OP_EQUALVERIFY operation"}
ERR_SIG_PUSHONLY = {"err_msg": "Only push operators allowed in signatures"}

DUST_LIMIT = 600
MIN_FEE = 50000

TX_STANDARD_VERSIONS = [1, 2, TX_MAX_STANDARD_VERSION]

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
    with open(dirname + ("/%s" % sha1), 'w') as f:
        f.write(dump)

# Data type to keep track of UTXOs, where they were created, and how to spend them.
UTXOData = namedtuple('UTXOData', 'outpoint,output,spender')


class SpenderTestMixin:
    """Mixin for a BitcoinTestFramework test that runs spenders against a node.

    The test must set self.nodesigner to a NodeSigner that has mature coins before calling test_spenders().
    """
    def add_options(self, parser):
        parser.add_argument("--dumptests", dest="dump_tests", default=False, action="store_true",
                            help="Dump generated test cases to directory set by TEST_DUMP_DIR environment variable")

    def block_submit(self, node, txs, msg, err_msg, cb_pubkey=None, fees=0, sigops_weight=0, witness=False, accept=False):

        # Deplete block of any non-tapscript sigops using a single additional 0-value coinbase output.
        # It is not impossible to fit enough tapscript sigops to hit the old 80k limit without
        # busting txin-level limits. We simply have to account for the p2pk outputs in all
        # transactions.
        extra_output_script = CScript(bytes([OP_CHECKSIG]*((MAX_BLOCK_SIGOPS_WEIGHT - sigops_weight) // WITNESS_SCALE_FACTOR)))

        coinbase_tx = create_coinbase(self.lastblockheight + 1, pubkey=cb_pubkey, extra_output_script=extra_output_script, fees=fees)
        block = create_block(self.tip, coinbase_tx, ntime=self.lastblocktime + 1, txlist=txs)
        witness and add_witness_commitment(block)
        block.solve()
        block_response = node.submitblock(block.serialize().hex())
        if err_msg is not None:
            assert block_response is not None and err_msg in block_response, "Missing error message '%s' from block response '%s': %s" % (err_msg, "(None)" if block_response is None else block_response, msg)
        if accept:
            assert node.getbestblockhash() == block.hash_hex, "Failed to accept: %s (response: %s)" % (msg, block_response)
            self.tip = block.hash_int
            self.lastblockhash = block.hash_hex
            self.lastblocktime += 1
            self.lastblockheight += 1
        else:
            assert node.getbestblockhash() == self.lastblockhash, "Failed to reject: " + msg

    def init_blockinfo(self, node):
        # Initialize variables used by block_submit().
        self.lastblockhash = node.getbestblockhash()
        self.tip = int(self.lastblockhash, 16)
        block = node.getblock(self.lastblockhash)
        self.lastblockheight = block['height']
        self.lastblocktime = block['time']

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
            pubkey, spk, _ = self.nodesigner.getnewaddress(address_type=random.choice(["legacy", "p2sh-segwit", "bech32"]))
            host_spks.append(spk)
            host_pubkeys.append(pubkey)

        self.init_blockinfo(node)

        # Create transactions spending up to 50 of the wallet's inputs, with one output for each spender, and
        # one change output at the end. The transaction is constructed on the Python side to enable
        # having multiple outputs to the same address and outputs with no assigned address. The node
        # is then asked to sign it through signrawtransactionwithkey, and then added to a block on the
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
            unspents = self.nodesigner.listunspent()
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
            # Ask the node to sign
            fund_tx = tx_from_hex(self.nodesigner.signrawtransaction(fund_tx.serialize().hex(), unspents)["hex"])
            # Construct UTXOData entries
            for i in range(count_this_tx):
                utxodata = UTXOData(outpoint=COutPoint(fund_tx.txid_int, i), output=fund_tx.vout[i], spender=spenders[done])
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
        assert_equal(done, len(normal_utxos) + len(mismatching_utxos))

        left = done
        while left:
            # Construct CTransaction with random version, nLocktime
            tx = CTransaction()
            tx.version = random.choice(TX_STANDARD_VERSIONS + [0, TX_MAX_STANDARD_VERSION + 1, random.getrandbits(32)])
            min_sequence = (tx.version != 1 and tx.version != 0) * 0x80000000  # The minimum sequence number to disable relative locktime
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
                is_standard_tx = (
                    fail_input is None  # Must be valid to be standard
                    and (all(utxo.spender.is_standard for utxo in input_utxos))  # All inputs must be standard
                    and tx.version in TX_STANDARD_VERSIONS # The tx version must be standard
                    # test doesn't cover in-mempool spends, so only TRUC_MAX_VSIZE limit is hit
                    and not (tx.version == 3 and tx.get_vsize() > TRUC_MAX_VSIZE)  # Topological standardness rules must be followed
                )
                msg = ','.join(utxo.spender.comment + ("*" if n == fail_input else "") for n, utxo in enumerate(input_utxos))
                if is_standard_tx:
                    node.sendrawtransaction(tx.serialize().hex(), 0)
                    assert node.getmempoolentry(tx.txid_hex) is not None, "Failed to accept into mempool: " + msg
                else:
                    assert_raises_rpc_error(-26, None, node.sendrawtransaction, tx.serialize().hex(), 0)
                # Submit in a block
                self.block_submit(node, [tx], msg, witness=True, accept=fail_input is None, cb_pubkey=cb_pubkey, fees=fee, sigops_weight=sigops_weight, err_msg=expected_fail_msg)

            if (len(spenders) - left) // 200 > (len(spenders) - left - len(input_utxos)) // 200:
                self.log.info("  - %i tests done" % (len(spenders) - left))

        assert_equal(left, 0)
        assert_equal(len(normal_utxos), 0)
        assert_equal(len(mismatching_utxos), 0)
        self.log.info("  - Done")
