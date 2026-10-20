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
from .script import ANNEX_TAG, CScript, hash160
from .script_util import is_p2pkh_script, is_p2sh_script, is_p2tr_script, is_p2wpkh_script, script_witness_version
from .test_node import TestNode

K_MAX = 2323


def get_input_pubkey(script_sig: CScript, witness: list[bytes], prevout_spk: bytes) -> GE | None:
    """Return the public key of an input as a group element, or None if the input is not eligible."""
    if is_p2tr_script(prevout_spk):
        stack = list(witness)
        if len(stack) > 1 and stack[-1][0] == ANNEX_TAG:
            _ = stack.pop()
        if len(stack) > 1 and stack[-1][1:33] == bytes.fromhex(H_POINT):
            return None
        return GE.from_bytes_xonly(prevout_spk[2:])
    if is_p2wpkh_script(prevout_spk) and len(witness[-1]) == 33:
        return GE.from_bytes(witness[-1])
    if is_p2sh_script(prevout_spk):
        pushes = list(script_sig)
        if len(pushes) == 1 and isinstance(pushes[0], bytes) and is_p2wpkh_script(pushes[0]) and len(witness[-1]) == 33:
            return GE.from_bytes(witness[-1])
        return None
    if is_p2pkh_script(prevout_spk):
        for push in script_sig:
            if isinstance(push, bytes) and len(push) == 33 and hash160(push) == prevout_spk[3:23]:
                return GE.from_bytes(push)
    return None


def scan(inputs: list[tuple[COutPoint, CScript, list[bytes], bytes]], output_spks: list[bytes], scan_privkey: bytes, spend_pubkey: bytes) -> list[dict[str, str]]:
    """Find the outputs paying to a silent payments recipient.

    inputs is a list of (COutPoint, scriptSig, witness stack, prevout scriptPubKey). Return a
    list of {"output": x-only output key, "tweak": spend key tweak}, both hex.
    """
    if any((script_witness_version(prevout_spk) or 0) >= 2 for *_, prevout_spk in inputs):
        return []
    pubkeys = [pubkey for pubkey in (get_input_pubkey(*i[1:]) for i in inputs) if pubkey is not None]
    A = sum(pubkeys, GE())
    if A.infinity:
        return []
    smallest_outpoint = min(outpoint.serialize() for outpoint, *_ in inputs)
    input_hash = int.from_bytes(TaggedHash("BIP0352/Inputs", smallest_outpoint + A.to_bytes_compressed()), "big")
    ecdh_shared_secret = (input_hash * int.from_bytes(scan_privkey, "big") % ORDER) * A
    B_spend = GE.from_bytes(spend_pubkey)
    outputs = {spk[2:] for spk in output_spks if is_p2tr_script(spk)}
    found = []
    for k in range(K_MAX):
        tweak = TaggedHash("BIP0352/SharedSecret", ecdh_shared_secret.to_bytes_compressed() + k.to_bytes(4, "big"))
        P_k = (B_spend + int.from_bytes(tweak, "big") * G).to_bytes_xonly()
        if P_k not in outputs:
            break
        found.append({"output": P_k.hex(), "tweak": tweak.hex()})
    return found


def scan_tx(node: TestNode, tx_hex: str, scan_privkey: bytes, spend_pubkey: bytes) -> list[dict[str, str]]:
    """Scan a transaction, looking up its prevouts in the UTXO set of node, or with getrawtransaction."""
    tx = from_hex(CTransaction(), tx_hex)
    inputs = []
    for i, txin in enumerate(tx.vin):
        txid, n = f"{txin.prevout.hash:064x}", txin.prevout.n
        prevout = node.gettxout(txid, n, include_mempool=False) or node.getrawtransaction(txid, True)["vout"][n]
        witness = tx.wit.vtxinwit[i].scriptWitness.stack if i < len(tx.wit.vtxinwit) else []
        inputs.append((txin.prevout, CScript(txin.scriptSig), witness, bytes.fromhex(prevout["scriptPubKey"]["hex"])))
    return scan(inputs, [txout.scriptPubKey for txout in tx.vout], scan_privkey, spend_pubkey)


class TestFrameworkBIP352(unittest.TestCase):
    def receiving_vectors(self):
        with (Path(__file__).parents[3] / "src/test/data/bip352_send_and_receive_vectors.json").open() as vectors_file:
            vectors = json.load(vectors_file)
        for test in vectors:
            for receiving in test["receiving"]:
                if not receiving["given"]["labels"]:
                    yield test["comment"], receiving["given"], receiving["expected"]

    def test_receiving_vectors(self):
        for comment, given, expected in self.receiving_vectors():
            inputs = [(
                COutPoint(int(vin["txid"], 16), vin["vout"]),
                CScript.fromhex(vin["scriptSig"]),
                deser_string_vector(BytesIO(bytes.fromhex(vin["txinwitness"]))) if vin["txinwitness"] else [],
                bytes.fromhex(vin["prevout"]["scriptPubKey"]["hex"]),
            ) for vin in given["vin"]]
            output_spks = [bytes.fromhex("5120" + output) for output in given["outputs"]]
            spend_privkey = int(given["key_material"]["spend_priv_key"], 16)
            found = scan(inputs, output_spks, bytes.fromhex(given["key_material"]["scan_priv_key"]),
                         (spend_privkey * G).to_bytes_compressed())
            expected_outputs = [{"output": output["pub_key"], "tweak": output["priv_key_tweak"]} for output in expected["outputs"]]
            self.assertEqual(sorted(found, key=lambda o: o["output"]), sorted(expected_outputs, key=lambda o: o["output"]), comment)
