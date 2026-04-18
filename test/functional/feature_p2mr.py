#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP360 draft P2MR mempool policy and script validation."""

from typing import Optional

from test_framework.key import TaggedHash
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    ser_string,
)
from test_framework.script import (
    CScript,
    LEAF_VERSION_TAPSCRIPT,
    OP_2,
    OP_TRUE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class P2MRTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    @staticmethod
    def p2mr_script_pubkey(script: bytes) -> CScript:
        # BIP360 draft: witness v2 program commits directly to TapLeaf/TapBranch root.
        root = TaggedHash("TapLeaf", bytes([LEAF_VERSION_TAPSCRIPT]) + ser_string(script))
        return CScript([OP_2, root])

    def build_spend(self, *, prev_txid: str, prev_vout: int, prev_value: int, witness_stack: Optional[list[bytes]]) -> CTransaction:
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(prev_txid, 16), prev_vout), nSequence=0xFFFFFFFD)]
        tx.vout = [CTxOut(prev_value - 1000, self.wallet.get_output_script())]
        tx.version = 2
        tx.nLockTime = 0
        if witness_stack is not None:
            tx.wit.vtxinwit = [CTxInWitness()]
            tx.wit.vtxinwit[0].scriptWitness.stack = witness_stack
        return tx

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        leaf_script = bytes(CScript([OP_TRUE]))
        p2mr_spk = self.p2mr_script_pubkey(leaf_script)

        self.log.info("Funding a witness-v2 P2MR output")
        funding = self.wallet.send_to(from_node=node, scriptPubKey=p2mr_spk, amount=50_000, fee=500)

        self.log.info("Missing witness must fail standardness for P2MR")
        no_witness_tx = self.build_spend(
            prev_txid=funding["txid"],
            prev_vout=funding["sent_vout"],
            prev_value=50_000,
            witness_stack=None,
        )
        res = node.testmempoolaccept([no_witness_tx.serialize().hex()])[0]
        assert_equal(res["allowed"], False)
        assert "bad-txns-nonstandard-inputs" in res["reject-reason"]

        self.log.info("Wrong control-byte parity must fail script verification")
        bad_control_tx = self.build_spend(
            prev_txid=funding["txid"],
            prev_vout=funding["sent_vout"],
            prev_value=50_000,
            witness_stack=[leaf_script, bytes([LEAF_VERSION_TAPSCRIPT])],  # 0xc0, parity bit unset
        )
        res = node.testmempoolaccept([bad_control_tx.serialize().hex()])[0]
        assert_equal(res["allowed"], False)
        self.log.info(f"Bad control reject reason: {res['reject-reason']}")
        assert (
            "bad-witness-nonstandard" in res["reject-reason"]
            or "bad-txns-nonstandard-inputs" in res["reject-reason"]
            or (
                "mempool-script-verify-flag-failed" in res["reject-reason"]
                and "Witness program hash mismatch" in res["reject-reason"]
            )
            or "witness program is undefined" in res["reject-reason"]
        )

        self.log.info("Valid P2MR witness must be accepted")
        good_control = bytes([LEAF_VERSION_TAPSCRIPT | 1])  # 0xc1, parity bit set
        good_tx = self.build_spend(
            prev_txid=funding["txid"],
            prev_vout=funding["sent_vout"],
            prev_value=50_000,
            witness_stack=[leaf_script, good_control],
        )
        res = node.testmempoolaccept([good_tx.serialize().hex()])[0]
        assert_equal(res["allowed"], True)

        self.log.info("Submitting valid P2MR spend")
        txid = node.sendrawtransaction(good_tx.serialize().hex())
        assert_equal(txid, good_tx.txid_hex)

        self.log.info("Mining block including P2MR spend")
        self.generate(node, 1)


if __name__ == '__main__':
    P2MRTest(__file__).main()
