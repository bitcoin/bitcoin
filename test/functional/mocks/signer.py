#!/usr/bin/env python3
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import sys
import argparse
import json

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

from test_framework.authproxy import AuthServiceProxy, JSONRPCException
from test_framework.psbt import (
    PSBT,
    PSBT_IN_PARTIAL_SIG,
    PSBT_IN_SIGHASH_TYPE,
    PSBT_IN_TAP_KEY_SIG,
    PSBT_OUT_AMOUNT,
    PSBT_OUT_SCRIPT,
)

# Master private key for the tpub in getdescriptors below. Used by signtx,
# which imports the keys into a wallet on the offline node provided by the
# test and lets it do the actual signing.
tprv = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK"

MOCK_WALLET = "mock"

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

def get_mock_wallet():
    """RPC connection to the wallet holding our private keys, created on
    first use. The test provides a dedicated offline node for it and passes
    the node's RPC URL via a file in our working directory."""
    with open(os.path.join(os.getcwd(), "mock_rpc_url"), "r", encoding="utf8") as f:
        node_url = f.read().strip()
    node = AuthServiceProxy(node_url)
    wallet = AuthServiceProxy(f"{node_url}/wallet/{MOCK_WALLET}")
    try:
        node.loadwallet(filename=MOCK_WALLET)
        return wallet
    except JSONRPCException as e:
        if e.error["code"] == -35:  # RPC_WALLET_ALREADY_LOADED
            return wallet
        if e.error["code"] != -18:  # RPC_WALLET_NOT_FOUND
            raise
    node.createwallet(wallet_name=MOCK_WALLET, blank=True)
    requests = []
    for desc in [f"pkh({tprv}/<0;1>/*)", f"sh(wpkh({tprv}/<0;1>/*))", f"wpkh({tprv}/<0;1>/*)", f"tr({tprv}/<0;1>/*)"]:
        checksum = node.getdescriptorinfo(descriptor=desc)["checksum"]
        requests.append({"desc": f"{desc}#{checksum}", "timestamp": "now", "range": [0, 99]})
    result = wallet.importdescriptors(requests=requests)
    assert all(r["success"] for r in result)
    return wallet

def tamper(psbt_b64, mode):
    """Alter the transaction described by the (version 2) PSBT before signing
    it, like a rogue or broken signer might."""
    psbt = PSBT.from_base64(psbt_b64)
    if mode == "change_amount":
        # Steal from the output by redirecting the value to fees
        amount = int.from_bytes(psbt.o[0].map[PSBT_OUT_AMOUNT], "little", signed=True)
        psbt.o[0].map[PSBT_OUT_AMOUNT] = (amount - 1).to_bytes(8, "little", signed=True)
    elif mode == "change_script":
        psbt.o[0].map[PSBT_OUT_SCRIPT] = bytes([0x51])  # OP_TRUE
    elif mode == "remove_output":
        psbt.o.pop()
    return psbt.to_base64()

def signtx(args):
    if args.fingerprint != "00000001":
        return sys.stdout.write(json.dumps({"error": "Unexpected fingerprint", "fingerprint": args.fingerprint}))

    # The test can instruct us to sign in a specific, possibly misbehaving, way
    mode = None
    sign_mode_path = os.path.join(os.getcwd(), "mock_sign_mode")
    if os.path.isfile(sign_mode_path):
        with open(sign_mode_path, "r", encoding="utf8") as f:
            mode = f.read().strip()

    psbt = args.psbt
    if mode in ("change_amount", "change_script", "remove_output"):
        psbt = tamper(psbt, mode)

    sign_options = {}
    if mode in ("sighash_none", "sighash_none_hidden"):
        sign_options["sighashtype"] = "NONE"
    elif mode == "sighash_all_anyonecanpay":
        sign_options["sighashtype"] = "ALL|ANYONECANPAY"

    result = get_mock_wallet().walletprocesspsbt(psbt=psbt, sign=True, bip32derivs=False, finalize=False, **sign_options)
    reply = result["psbt"]

    if mode == "sighash_none_hidden":
        # Drop the declared sighash type, leaving only the signatures
        # themselves to reveal it
        signed = PSBT.from_base64(reply)
        for psbt_in in signed.i:
            psbt_in.map.pop(PSBT_IN_SIGHASH_TYPE, None)
        reply = signed.to_base64()
    elif mode == "strip":
        # Return only the signatures, plus the fields required to describe
        # the same transaction
        signed = PSBT.from_base64(reply)
        stripped = PSBT.from_base64(reply)
        stripped.make_blank()
        for signed_in, stripped_in in zip(signed.i, stripped.i):
            for key, value in signed_in.map.items():
                if key == PSBT_IN_TAP_KEY_SIG or (isinstance(key, bytes) and key[0] == PSBT_IN_PARTIAL_SIG):
                    stripped_in.map[key] = value
        reply = stripped.to_base64()

    sys.stdout.write(json.dumps({"psbt": reply}))

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
