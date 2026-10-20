#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""BIP352 silent payments scanning, without labels."""

from io import BytesIO
import json
from pathlib import Path
import unittest

from .crypto.secp256k1 import G, GE
from .key import H_POINT, ORDER, TaggedHash
from .messages import COutPoint, CTransaction, deser_string_vector, from_hex
from .script import hash160

K_MAX = 2323


def get_input_pubkey(script_sig, witness, prevout_spk):
    """Return the public key of an input as a GE, or None if the input is not eligible."""
    if len(prevout_spk) == 34 and prevout_spk[:2] == b"\x51\x20":
        stack = list(witness)
        if len(stack) > 1 and stack[-1][:1] == b"\x50":
            stack.pop()
        if len(stack) > 1 and stack[-1][1:33] == bytes.fromhex(H_POINT):
            return None
        return GE.from_bytes_xonly(prevout_spk[2:])
    if len(prevout_spk) == 22 and prevout_spk[:2] == b"\x00\x14":
        if witness and len(witness[-1]) == 33:
            return GE.from_bytes(witness[-1])
        return None
    if len(prevout_spk) == 23 and prevout_spk[:2] == b"\xa9\x14":
        if script_sig[:3] == b"\x16\x00\x14" and len(script_sig) == 23 and witness and len(witness[-1]) == 33:
            return GE.from_bytes(witness[-1])
        return None
    if len(prevout_spk) == 25 and prevout_spk[:3] == b"\x76\xa9\x14" and prevout_spk[23:] == b"\x88\xac":
        for i in range(len(script_sig), 32, -1):
            pubkey = script_sig[i - 33:i]
            if hash160(pubkey) == prevout_spk[3:23]:
                return GE.from_bytes(pubkey)
    return None


def scan(inputs, output_spks, scan_privkey, spend_pubkey):
    """Find the outputs paying to a silent payments recipient.

    inputs is a list of (COutPoint, scriptSig, witness stack, prevout scriptPubKey). Return a
    list of {"output": x-only output key, "tweak": spend key tweak}, both hex.
    """
    pubkeys = [pubkey for pubkey in (get_input_pubkey(*i[1:]) for i in inputs) if pubkey is not None]
    A = sum(pubkeys, GE())
    if A.infinity:
        return []
    smallest_outpoint = min(outpoint.serialize() for outpoint, *_ in inputs)
    input_hash = int.from_bytes(TaggedHash("BIP0352/Inputs", smallest_outpoint + A.to_bytes_compressed()), "big")
    ecdh_shared_secret = (input_hash * int.from_bytes(scan_privkey, "big") % ORDER) * A
    B_spend = GE.from_bytes(spend_pubkey)
    outputs = {spk[2:] for spk in output_spks if len(spk) == 34 and spk[:2] == b"\x51\x20"}
    found = []
    for k in range(K_MAX):
        tweak = TaggedHash("BIP0352/SharedSecret", ecdh_shared_secret.to_bytes_compressed() + k.to_bytes(4, "big"))
        P_k = (B_spend + int.from_bytes(tweak, "big") * G).to_bytes_xonly()
        if P_k not in outputs:
            break
        found.append({"output": P_k.hex(), "tweak": tweak.hex()})
    return found


def scan_tx(node, tx_hex, scan_privkey, spend_pubkey):
    """Scan a transaction, looking up its prevouts in the UTXO set of node, or with getrawtransaction."""
    tx = from_hex(CTransaction(), tx_hex)
    inputs = []
    for i, txin in enumerate(tx.vin):
        txid, n = f"{txin.prevout.hash:064x}", txin.prevout.n
        prevout = node.gettxout(txid, n, include_mempool=False) or node.getrawtransaction(txid, True)["vout"][n]
        witness = tx.wit.vtxinwit[i].scriptWitness.stack if i < len(tx.wit.vtxinwit) else []
        inputs.append((txin.prevout, txin.scriptSig, witness, bytes.fromhex(prevout["scriptPubKey"]["hex"])))
    return scan(inputs, [txout.scriptPubKey for txout in tx.vout], scan_privkey, spend_pubkey)


class TestFrameworkBIP352(unittest.TestCase):
    def test_receiving_vectors(self):
        with (Path(__file__).parents[3] / "src/test/data/bip352_send_and_receive_vectors.json").open() as vectors_file:
            for test in json.load(vectors_file):
                for receiving in test["receiving"]:
                    given = receiving["given"]
                    if given["labels"]:
                        continue
                    inputs = [(
                        COutPoint(int(vin["txid"], 16), vin["vout"]),
                        bytes.fromhex(vin["scriptSig"]),
                        deser_string_vector(BytesIO(bytes.fromhex(vin["txinwitness"]))) if vin["txinwitness"] else [],
                        bytes.fromhex(vin["prevout"]["scriptPubKey"]["hex"]),
                    ) for vin in given["vin"]]
                    output_spks = [bytes.fromhex("5120" + output) for output in given["outputs"]]
                    spend_privkey = int(given["key_material"]["spend_priv_key"], 16)
                    found = scan(inputs, output_spks, bytes.fromhex(given["key_material"]["scan_priv_key"]),
                                 (spend_privkey * G).to_bytes_compressed())
                    expected = [{"output": output["pub_key"], "tweak": output["priv_key_tweak"]} for output in receiving["expected"]["outputs"]]
                    self.assertEqual(sorted(found, key=lambda o: o["output"]), sorted(expected, key=lambda o: o["output"]), test["comment"])
