#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test scantxforsilentpayments RPC.

Exercises parameter validation, prevout resolution (UTXO set, mempool,
txindex), and adversarial/malformed input scripts.
"""
from test_framework.key import (
    ECKey,
    compute_xonly_pubkey,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_1,
    OP_IF,
    sign_input_legacy,
    taproot_construct,
)
from test_framework.script_util import (
    key_to_p2pkh_script,
    key_to_p2wpkh_script,
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    sync_txindex,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair

# A structurally valid taproot output; content is irrelevant to the tests below
# (an all-zero x-only key is not on the curve, so it's simply skipped when
# actually matching outputs -- see bip352::ScanForSilentPaymentsOutputs).
DUMMY_TAPROOT_SPK = CScript([OP_1, b"\x00" * 32])

NEVER_EXISTED_OUTPOINT = COutPoint(int("ff" * 32, 16), 0)


class ScanTxForSilentPaymentsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-txindex=1", "-acceptnonstdtxn=1"]]

    def setup_network(self):
        self.setup_nodes()

    def fund_p2pkh(self, *, mine):
        """Create a fresh P2PKH output, optionally mining it. Returns (privkey, pubkey, outpoint, amount)."""
        privkey, pubkey = generate_keypair()
        spk = key_to_p2pkh_script(pubkey)
        amount = 100_000
        res = self.wallet.send_to(from_node=self.nodes[0], scriptPubKey=spk, amount=amount)
        outpoint = COutPoint(int(res["txid"], 16), res["sent_vout"])
        if mine:
            self.generate(self.nodes[0], 1)
        return privkey, pubkey, outpoint, amount

    def build_p2pkh_spend(self, privkey, pubkey, outpoint, in_amount, *, vout=None):
        """Build (but do not broadcast) a fully-signed tx spending outpoint."""
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint)]
        tx.vout = vout if vout is not None else [CTxOut(in_amount - 1000, DUMMY_TAPROOT_SPK)]
        spk = key_to_p2pkh_script(pubkey)
        tx.vin[0].scriptSig = CScript([pubkey])
        sign_input_legacy(tx, 0, spk, privkey)
        return tx

    def test_parameter_validation(self):
        self.log.info("Testing scantxforsilentpayments parameter validation")
        node = self.nodes[0]
        dummy_tx = CTransaction()
        dummy_tx.vin = [CTxIn(COutPoint(0, 0))]
        dummy_tx.vout = [CTxOut(0, DUMMY_TAPROOT_SPK)]
        dummy_tx_hex = dummy_tx.serialize().hex()

        self.log.info("  Invalid tx hex")
        assert_raises_rpc_error(-22, "TX decode failed", node.scantxforsilentpayments, "not hex", self.scan_key_hex, self.spend_pubkey_hex)
        assert_raises_rpc_error(-22, "TX decode failed", node.scantxforsilentpayments, "deadbeef", self.scan_key_hex, self.spend_pubkey_hex)

        self.log.info("  Invalid scan_key")
        assert_raises_rpc_error(-8, "scan_key must be 32 bytes", node.scantxforsilentpayments, dummy_tx_hex, "aa" * 31, self.spend_pubkey_hex)
        assert_raises_rpc_error(-8, "scan_key must be 32 bytes", node.scantxforsilentpayments, dummy_tx_hex, "aa" * 33, self.spend_pubkey_hex)
        assert_raises_rpc_error(-8, "scan_key must be 32 bytes", node.scantxforsilentpayments, dummy_tx_hex, "zz" * 32, self.spend_pubkey_hex)
        assert_raises_rpc_error(-8, "Invalid scan_key", node.scantxforsilentpayments, dummy_tx_hex, "00" * 32, self.spend_pubkey_hex)
        assert_raises_rpc_error(-8, "Invalid scan_key", node.scantxforsilentpayments, dummy_tx_hex, "ff" * 32, self.spend_pubkey_hex)

        self.log.info("  Invalid spend_pubkey")
        assert_raises_rpc_error(-8, "spend_pubkey must be 33 bytes", node.scantxforsilentpayments, dummy_tx_hex, self.scan_key_hex, "aa" * 32)
        assert_raises_rpc_error(-8, "spend_pubkey must be 33 bytes", node.scantxforsilentpayments, dummy_tx_hex, self.scan_key_hex, "aa" * 34)
        assert_raises_rpc_error(-8, "Invalid spend_pubkey", node.scantxforsilentpayments, dummy_tx_hex, self.scan_key_hex, "02" + "00" * 32)
        assert_raises_rpc_error(-8, "Invalid spend_pubkey", node.scantxforsilentpayments, dummy_tx_hex, self.scan_key_hex, "04" + "11" * 32)

        self.log.info("  Missing arguments")
        assert_raises_rpc_error(-1, "scantxforsilentpayments", node.scantxforsilentpayments, dummy_tx_hex)
        assert_raises_rpc_error(-1, "scantxforsilentpayments", node.scantxforsilentpayments, dummy_tx_hex, self.scan_key_hex)

    def test_prevout_not_found(self):
        self.log.info("Testing scantxforsilentpayments with a prevout that doesn't exist")
        tx = CTransaction()
        tx.vin = [CTxIn(NEVER_EXISTED_OUTPOINT)]
        tx.vout = [CTxOut(0, DUMMY_TAPROOT_SPK)]
        assert_raises_rpc_error(-1, "Could not find prevout", self.nodes[0].scantxforsilentpayments,
                                 tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex)

    def test_prevout_resolution(self):
        self.log.info("Testing scantxforsilentpayments resolves prevouts from the UTXO set, mempool, and txindex")
        node = self.nodes[0]

        self.log.info("  Prevout confirmed and unspent (UTXO set)")
        privkey, pubkey, outpoint, amount = self.fund_p2pkh(mine=True)
        tx = self.build_p2pkh_spend(privkey, pubkey, outpoint, amount)
        result = node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex)
        assert isinstance(result, list)

        self.log.info("  Prevout only exists unconfirmed in the mempool")
        privkey, pubkey, outpoint, amount = self.fund_p2pkh(mine=False)
        tx = self.build_p2pkh_spend(privkey, pubkey, outpoint, amount)
        result = node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex)
        assert isinstance(result, list)

        self.log.info("  Prevout confirmed and already spent by a real, mined transaction (txindex)")
        privkey, pubkey, outpoint, amount = self.fund_p2pkh(mine=True)
        spend_tx = self.build_p2pkh_spend(privkey, pubkey, outpoint, amount)
        spend_txid = node.sendrawtransaction(spend_tx.serialize().hex())
        self.generate(node, 1)
        sync_txindex(self, node)
        spent_prevout_tx_hex = node.getrawtransaction(spend_txid)
        result = node.scantxforsilentpayments(spent_prevout_tx_hex, self.scan_key_hex, self.spend_pubkey_hex)
        assert isinstance(result, list)

        self.log.info("  Without -txindex, the same confirmed-and-spent prevout cannot be resolved")
        self.restart_node(0, extra_args=["-acceptnonstdtxn=1"])
        assert_raises_rpc_error(-1, "Could not find prevout", node.scantxforsilentpayments,
                                 spent_prevout_tx_hex, self.scan_key_hex, self.spend_pubkey_hex)
        self.restart_node(0, extra_args=self.extra_args[0])
        self.wallet.rescan_utxos()

    def test_no_elligible_inputs(self):
        self.log.info("Testing scantxforsilentpayments with no eligible inputs")
        node = self.nodes[0]

        # Create a transaction that spends a p2wsh script
        redeem_script = CScript([OP_1, b"\x02" * 33])
        wsh_spk = script_to_p2wsh_script(redeem_script)
        res = self.wallet.send_to(from_node=node, scriptPubKey=wsh_spk, amount=100_000)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(res["txid"], 16), res["sent_vout"]))]
        tx.vout = [CTxOut(99_000, DUMMY_TAPROOT_SPK)]
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [redeem_script]
        assert_equal(node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex), [])

    def test_invalid_scripts(self):
        self.log.info("Testing invalid/non-standard prevout scripts does not crash node")
        node = self.nodes[0]

        self.log.info("  Non-standard prevout scriptPubKey")
        garbage_spk = CScript([b"\xab" * 4, b"\xcd" * 4])
        res = self.wallet.send_to(from_node=node, scriptPubKey=garbage_spk, amount=100_000)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(res["txid"], 16), res["sent_vout"]))]
        tx.vout = [CTxOut(99_000, DUMMY_TAPROOT_SPK)]
        assert_equal(node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex), [])

        self.log.info("  P2WPKH prevout spent with an empty witness")
        privkey, pubkey = generate_keypair()
        spk = key_to_p2wpkh_script(pubkey)
        res = self.wallet.send_to(from_node=node, scriptPubKey=spk, amount=100_000)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(res["txid"], 16), res["sent_vout"]))]
        tx.vout = [CTxOut(99_000, DUMMY_TAPROOT_SPK)]
        tx.wit.vtxinwit = [CTxInWitness()]  # empty stack
        assert_equal(node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex), [])

        self.log.info("  Taproot script-path spend with a truncated control block")
        privkey, _ = generate_keypair()
        internal_key, _ = compute_xonly_pubkey(privkey.get_bytes())
        leaf_script = CScript([OP_1])
        info = taproot_construct(internal_key, [("leaf", leaf_script)])
        res = self.wallet.send_to(from_node=node, scriptPubKey=info.scriptPubKey, amount=100_000)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(res["txid"], 16), res["sent_vout"]))]
        tx.vout = [CTxOut(99_000, DUMMY_TAPROOT_SPK)]
        tx.wit.vtxinwit = [CTxInWitness()]
        leaf = info.leaves["leaf"]
        full_control_block = bytes([leaf.version | info.negflag]) + info.internal_pubkey + leaf.merklebranch
        short_control_block = full_control_block[:20]  # < 33 bytes
        tx.wit.vtxinwit[0].scriptWitness.stack = [leaf.script, short_control_block]
        assert_equal(node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex), [])

        self.log.info("  P2PKH scriptSig that fails to evaluate")
        _, pubkey = generate_keypair()
        spk = key_to_p2pkh_script(pubkey)
        res = self.wallet.send_to(from_node=node, scriptPubKey=spk, amount=100_000)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(res["txid"], 16), res["sent_vout"]))]
        tx.vout = [CTxOut(99_000, DUMMY_TAPROOT_SPK)]
        tx.vin[0].scriptSig = CScript([OP_IF])  # unbalanced conditional -> EvalScript fails
        assert_equal(node.scantxforsilentpayments(tx.serialize().hex(), self.scan_key_hex, self.spend_pubkey_hex), [])

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.scan_key = ECKey()
        self.scan_key.generate()
        self.spend_key = ECKey()
        self.spend_key.generate()
        self.scan_key_hex = self.scan_key.get_bytes().hex()
        self.spend_pubkey_hex = self.spend_key.get_pubkey().get_bytes().hex()

        self.test_parameter_validation()
        self.test_prevout_not_found()
        self.test_prevout_resolution()
        self.test_no_elligible_inputs()
        self.test_invalid_scripts()


if __name__ == '__main__':
    ScanTxForSilentPaymentsTest(__file__).main()
