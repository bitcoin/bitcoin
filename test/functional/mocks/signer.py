#!/usr/bin/env python3
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import sys
import argparse
import json

def perform_pre_checks():
    mock_result_path = os.path.join(os.getcwd(), "mock_result")
    if os.path.isfile(mock_result_path):
        with open(mock_result_path, "r") as f:
            mock_result = f.read()
        if mock_result[0]:
            sys.stdout.write(mock_result[2:])
            sys.exit(int(mock_result[0]))

def enumerate(args):
    sys.stdout.write(json.dumps([{"fingerprint": "00000001", "type": "trezor", "model": "trezor_t"}]))

def getdescriptors(args):
    xpub = "tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B"

    sys.stdout.write(json.dumps({
        "receive": [
            "pkh([00000001/44h/1h/" + args.account + "']" + xpub + "/0/*)#aqllu46s",
            "sh(wpkh([00000001/49h/1h/" + args.account + "']" + xpub + "/0/*))#5dh56mgg",
            "wpkh([00000001/84h/1h/" + args.account + "']" + xpub + "/0/*)#h62dxaej",
            "tr([00000001/86h/1h/" + args.account + "']" + xpub + "/0/*)#pcd5w87f"
        ],
        "internal": [
            "pkh([00000001/44h/1h/" + args.account + "']" + xpub + "/1/*)#v567pq2g",
            "sh(wpkh([00000001/49h/1h/" + args.account + "']" + xpub + "/1/*))#pvezzyah",
            "wpkh([00000001/84h/1h/" + args.account + "']" + xpub + "/1/*)#xw0vmgf2",
            "tr([00000001/86h/1h/" + args.account + "']" + xpub + "/1/*)#svg4njw3"

        ]
    }))


def displayaddress(args):
    if args.fingerprint != "00000001":
        return sys.stdout.write(json.dumps({"error": "Unexpected fingerprint", "fingerprint": args.fingerprint}))

    expected_desc = {
        "wpkh([00000001/84h/1h/0h/0/0]02c97dc3f4420402e01a113984311bf4a1b8de376cac0bdcfaf1b3ac81f13433c7)#3te6hhy7": "bcrt1qm90ugl4d48jv8n6e5t9ln6t9zlpm5th68x4f8g",
        "sh(wpkh([00000001/49h/1h/0h/0/0]02c97dc3f4420402e01a113984311bf4a1b8de376cac0bdcfaf1b3ac81f13433c7))#kz9y5w82": "2N2gQKzjUe47gM8p1JZxaAkTcoHPXV6YyVp",
        "pkh([00000001/44h/1h/0h/0/0]02c97dc3f4420402e01a113984311bf4a1b8de376cac0bdcfaf1b3ac81f13433c7)#q3pqd8wh": "n1LKejAadN6hg2FrBXoU1KrwX4uK16mco9",
        "tr([00000001/86h/1h/0h/0/0]c97dc3f4420402e01a113984311bf4a1b8de376cac0bdcfaf1b3ac81f13433c7)#puqqa90m": "tb1phw4cgpt6cd30kz9k4wkpwm872cdvhss29jga2xpmftelhqll62mscq0k4g",
        "wpkh([00000001/84h/1h/0h/0/1]03a20a46308be0b8ded6dff0a22b10b4245c587ccf23f3b4a303885be3a524f172)#aqpjv5xr": "wrong_address",
    }
    if args.desc not in expected_desc:
        return sys.stdout.write(json.dumps({"error": "Unexpected descriptor", "desc": args.desc}))

    return sys.stdout.write(json.dumps({"address": expected_desc[args.desc]}))

def _signtx_rogue(psbt_b64, mode):
    """Corrupt a PSBT according to `mode` and return it, simulating a rogue signer."""
    import struct
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
    from test_framework.messages import CTransaction, from_binary
    from test_framework.psbt import (
        PSBT,
        PSBT_GLOBAL_UNSIGNED_TX,
        PSBT_IN_PARTIAL_SIG,
        PSBT_IN_SIGHASH_TYPE,
        PSBT_IN_TAP_KEY_SIG,
        PSBT_IN_TAP_SCRIPT_SIG,
        PSBT_OUT_AMOUNT,
        PSBT_OUT_SCRIPT,
    )

    psbt = PSBT.from_base64(psbt_b64)
    # 65-byte fake sig: 64 zero bytes + SIGHASH_NONE (0x02) — only length and
    # last byte are inspected by the invariant check, not signature validity.
    fake_tap_sig_unsafe_sighash = bytes(64) + bytes([0x02])

    if mode == "change_amount":
        if PSBT_GLOBAL_UNSIGNED_TX in psbt.g.map:   # PSBTv0
            tx = from_binary(CTransaction, psbt.g.map[PSBT_GLOBAL_UNSIGNED_TX])
            tx.vout[0].nValue += 1
            psbt.g.map[PSBT_GLOBAL_UNSIGNED_TX] = tx.serialize_without_witness()
        else:                                         # PSBTv2
            amount = struct.unpack("<q", psbt.o[0].map[PSBT_OUT_AMOUNT])[0]
            psbt.o[0].map[PSBT_OUT_AMOUNT] = struct.pack("<q", amount + 1)
    elif mode == "change_script":
        if PSBT_GLOBAL_UNSIGNED_TX in psbt.g.map:
            tx = from_binary(CTransaction, psbt.g.map[PSBT_GLOBAL_UNSIGNED_TX])
            tx.vout[0].scriptPubKey = bytes([0x51])  # OP_TRUE
            psbt.g.map[PSBT_GLOBAL_UNSIGNED_TX] = tx.serialize_without_witness()
        else:
            psbt.o[0].map[PSBT_OUT_SCRIPT] = bytes([0x51])
    elif mode == "remove_output":
        # Drop the last output so the reply has fewer outputs than the original.
        if PSBT_GLOBAL_UNSIGNED_TX in psbt.g.map:   # PSBTv0
            tx = from_binary(CTransaction, psbt.g.map[PSBT_GLOBAL_UNSIGNED_TX])
            tx.vout.pop()
            psbt.g.map[PSBT_GLOBAL_UNSIGNED_TX] = tx.serialize_without_witness()
        psbt.o.pop()                                  # PSBTv2 count is recomputed on serialize
    elif mode == "sighash_none":
        # Inject SIGHASH_NONE (2) via the per-input sighash_type field (BIP-174 key 0x03).
        psbt.i[0].map[PSBT_IN_SIGHASH_TYPE] = struct.pack("<I", 2)
    elif mode == "tap_key_sig_unsafe":
        # 65-byte taproot keypath sig whose explicit sighash byte is SIGHASH_NONE.
        psbt.i[0].map[PSBT_IN_TAP_KEY_SIG] = fake_tap_sig_unsafe_sighash
    elif mode == "tap_script_sig_unsafe":
        # PSBT_IN_TAP_SCRIPT_SIG key is <type><xonly(32)><leaf_hash(32)>.
        # Zero-filled placeholders are fine; the check only looks at sig length and last byte.
        key = bytes([PSBT_IN_TAP_SCRIPT_SIG]) + bytes(32) + bytes(32)
        psbt.i[0].map[key] = fake_tap_sig_unsafe_sighash
    elif mode == "ecdsa_partial_sig_unsafe":
        # Minimal valid strict-DER ECDSA sig (r=1, s=1) trailed by SIGHASH_NONE (0x02)
        # so the PSBT deserializer's CheckSignatureEncoding passes and the
        # invariant check sees the unsafe sighash byte. Pubkey is the secp256k1
        # generator G — a fully-valid compressed pubkey.
        pubkey = bytes.fromhex("0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798")
        key = bytes([PSBT_IN_PARTIAL_SIG]) + pubkey
        psbt.i[0].map[key] = bytes.fromhex("3006020101020101") + bytes([0x02])
    else:
        sys.stdout.write(json.dumps({"error": f"Unknown rogue mode: {mode}"}))
        return

    sys.stdout.write(json.dumps({"psbt": psbt.to_base64(), "complete": True}))


def signtx(args):
    if args.fingerprint != "00000001":
        return sys.stdout.write(json.dumps({"error": "Unexpected fingerprint", "fingerprint": args.fingerprint}))

    rogue_mode_path = os.path.join(os.getcwd(), "mock_rogue_mode")
    if os.path.isfile(rogue_mode_path):
        with open(rogue_mode_path) as f:
            mode = f.read().strip()
        _signtx_rogue(args.psbt, mode)
        return

    with open(os.path.join(os.getcwd(), "mock_psbt"), "r") as f:
        mock_psbt = f.read()

    if args.fingerprint == "00000001":
        sys.stdout.write(json.dumps({"psbt": mock_psbt, "complete": True}))
    else:
        sys.stdout.write(json.dumps({"psbt": args.psbt}))

parser = argparse.ArgumentParser(prog='./signer.py', description='External signer mock')
parser.add_argument('--fingerprint')
parser.add_argument('--chain', default='main')
parser.add_argument('--stdin', action='store_true')

subparsers = parser.add_subparsers(description='Commands', dest='command')
subparsers.required = True

parser_enumerate = subparsers.add_parser('enumerate', help='list available signers')
parser_enumerate.set_defaults(func=enumerate)

parser_getdescriptors = subparsers.add_parser('getdescriptors')
parser_getdescriptors.set_defaults(func=getdescriptors)
parser_getdescriptors.add_argument('--account', metavar='account')

parser_displayaddress = subparsers.add_parser('displayaddress', help='display address on signer')
parser_displayaddress.add_argument('--desc', metavar='desc')
parser_displayaddress.set_defaults(func=displayaddress)

parser_signtx = subparsers.add_parser('signtx')
parser_signtx.add_argument('psbt', metavar='psbt')

parser_signtx.set_defaults(func=signtx)

if not sys.stdin.isatty():
    buffer = sys.stdin.read()
    if buffer and buffer.rstrip() != "":
        sys.argv.extend(buffer.rstrip().split(" "))

args = parser.parse_args()

perform_pre_checks()

args.func(args)
